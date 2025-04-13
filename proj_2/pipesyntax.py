import json

from preprocessing import Database
from project import logger
from streamlit_flow.elements import StreamlitFlowNode, StreamlitFlowEdge


class QueryPlanNode:
    def __init__(self, node_data: dict) -> None:
        self.node_type = node_data.get("Node Type")
        self.startup_cost = node_data.get("Startup Cost")
        self.total_cost = node_data.get("Total Cost")
        self.relation_name = node_data.get("Relation Name")
        self.sort_key = node_data.get("Sort Key")

        self.children = []
        self.data = node_data  # Store the entire node data for potential future use

    def add_child(self, child_node: "QueryPlanNode") -> None:
        self.children.append(child_node)

    def __str__(self, level: int = 0) -> str:
        ret = "|>" * level + repr(self.node_type) + " (Cost: " + str(self.total_cost) + ")\n"
        for child in self.children:
            ret += child.__str__(level + 1)
        return ret


class QueryExecutionPlanGraph:
    def __init__(self, query: str, qep: str) -> None:
        self.query = query
        self.qep = qep
        self.root: QueryPlanNode

    def _qep_to_json(self) -> dict:
        return json.loads(self.qep)[0]["Plan"]

    def _parse(self) -> None:
        qep_json = self._qep_to_json()
        self.root = self._parse_node(qep_json)

    def _parse_node(self, qep_json: dict) -> QueryPlanNode:
        """Recursively parse the QEP JSON into a tree structure."""
        node = QueryPlanNode(qep_json)
        if "Plans" in qep_json:
            for child_data in qep_json["Plans"]:
                child_node = self._parse_node(child_data)
                node.add_child(child_node)
        return node

    def generate(self) -> None:
        """
        Generate the execution plan graph from the QEP.
        """
        self._parse()

    def visualize(self) -> tuple[list["StreamlitFlowNode"], list["StreamlitFlowEdge"]]:
        """
        Generate Streamlit Flow nodes & edges to be visualized on the interface.
        """
        nodes = []
        edges = []
        # mutable counter to maintain state during recursion
        node_counter = [1]

        def _build_flow_nodes_and_edges(node: QueryPlanNode, parent_id: str = None) -> str:
            node_id = str(node_counter[0])
            node_counter[0] += 1

            label = f"<h5>{node.node_type}</h5>"
            # TODO finalize fields that we want to keep
            # YJ: ill figure out a better way to do this & refactor
            if node.data.get("Relation Name"):
                label += f"on {node.data.get('Relation Name')}"
                if node.data.get("Alias") and node.data.get("Relation Name") != node.data.get("Alias"):
                    label += f" {node.data.get('Alias')}"
                label += "<br>"
            if node.data.get("Join Type"):
                label += f"{node.data.get('Join Type')} join<br>"
            if node.data.get("Hash Cond"):
                label += f"on {node.data.get('Hash Cond')}<br>"

            if node.data.get("Total Cost"):
                label += f"💲 <b>Cost:</b> {node.data.get('Total Cost')}<br>"
            if node.data.get("Actual Total Time"):
                label += f"⌛ <b>Time:</b> {node.data.get('Actual Total Time')}<br>"

            label += "<details><summary>Details</summary><sup>more details here if we want</sup></details>"  # TODO details

            flow_node = StreamlitFlowNode(
                id=node_id,
                # Position will be handled by frontend layout engine if (0,0)
                pos=(0, 0),
                data={"content": label},
                node_type='input' if not parent_id else (
                    'default' if node.children else 'output'),
                source_position='right',
                target_position='left',
                width=200,
                style={'width': '200px'}
            )
            nodes.append(flow_node)

            if parent_id:
                edge_id = f"{parent_id}-{node_id}"
                edge = StreamlitFlowEdge(
                    id=edge_id, source=parent_id, target=node_id)
                edges.append(edge)

            for child in node.children:
                _build_flow_nodes_and_edges(child, node_id)

        _build_flow_nodes_and_edges(self.root)
        return nodes, edges

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({self.query})"


class PipeSyntax:
    """PipeSyntax class to represent a SQL query in pipe syntax.

    This class holds the SQL query and provides methods to generate the pipe syntax.
    """

    def __init__(self, qep: QueryExecutionPlanGraph) -> None:
        self.qep = qep
        self.query = qep.query
        self.pipesyntax: str

    def generate(self) -> str:
        """
        Generate the pipe syntax from the query.
        """
        if not hasattr(self, "pipesyntax"):
            logger.info(f"Generating pipe syntax for query: {self.query}")
            self.qep.generate()
            self.pipesyntax = self._convert_to_pipe_syntax()
        return self.pipesyntax

    def _traverse_bottom_up(self, node: QueryPlanNode, result_list: list) -> None:
        """
        Traverses the query plan graph in a bottom-up manner.
        """
        for child in node.children:
            self._traverse_bottom_up(child, result_list)
        result_list.append(node.data)

    def _generate_bottom_up_output(self, root_node: QueryPlanNode) -> list:
        """
        Generates a bottom-up output from the query plan graph.
        """
        result_list = []
        self._traverse_bottom_up(root_node, result_list)
        return result_list

    def _convert_to_pipe_syntax(self) -> str:
        """
        Convert the qep to pipe syntax.
        """
        parse_list = self._generate_bottom_up_output(self.qep.root)
        ret = ""
        for node in parse_list:
            node_type = node.get("Node Type")
            if node_type == "Sort":
                ret += f"{node_type} sort on {node.get('Sort Key')} (Cost: {node.get('Total Cost')})\n"
            elif node_type == "Seq Scan" or node_type == "Bitmap Heap Scan":
                ret += f"{node_type} on {node.get('Relation Name')} (Cost: {node.get('Total Cost')})\n"
            else:
                ret += f"{node_type} (Cost: {node.get('Total Cost')})\n"
            ret += "|> "
        ret = ret[:-4]
        ret += ";"
        ret += "\n"
        return ret

    def __str__(self) -> str:
        if not hasattr(self, "pipesyntax"):
            self.generate()
        return self.pipesyntax

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({self.query})"


class PipeSyntaxParser:
    def __init__(self, database: Database) -> None:
        self.database = database
        self.current_query = None
        self.qep_graph: QueryExecutionPlanGraph
        self.pipesyntax: PipeSyntax

        logger.info("PipeSyntaxParser initialized with database.")

    def _get_qep(self, query: str) -> str:
        # qep = self.database.get_qep(query)
        # TODO change this when database works
        with open("sql_qep.txt", "r") as file:
            qep = file.read()
        return qep

    def _compute_query(self, query: str) -> None:
        qep = self._get_qep(query)
        self.qep_graph = QueryExecutionPlanGraph(query, qep)
        self.qep_graph.generate()
        self.pipesyntax = PipeSyntax(self.qep_graph)
        self.pipesyntax.generate()

    def get_pipe_syntax(self, query: str) -> PipeSyntax:
        """
        Pipe syntax given a query string.

        Args:
            query (str): The query string to check for pipe syntax.

        Returns:
            str: "Pipe" if the query contains pipe syntax, otherwise "No Pipe".
        """
        if self.current_query == query:
            return self.pipesyntax

        self.current_query = query
        logger.info(f"Getting pipe syntax for query: {query}")
        self._compute_query(query)
        return self.pipesyntax

    def get_execution_plan_graph(self, query: str) -> QueryExecutionPlanGraph:
        """
        Get the execution plan for a given query.

        Args:
            query (str): The query string to generate the execution plan for.

        Returns:
            QueryExecutionPlanGraph: The execution plan object.
        """
        if self.current_query == query:
            return self.qep_graph

        self.current_query = query
        logger.info(f"Getting qep for query: {query}")
        self._compute_query(query)
        return self.qep_graph


if __name__ == "__main__":
    # Example usage
    db = Database()
    parser = PipeSyntaxParser(db)
    query = "some query"
    pipe_syntax = parser.get_pipe_syntax(query)
    print(pipe_syntax)

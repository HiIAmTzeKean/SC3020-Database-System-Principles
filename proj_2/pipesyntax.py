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
        ret = "|>" * level + repr(self.node_type) + \
            " (Cost: " + str(self.total_cost) + ")\n"
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

            # get node details
            label_lines = [f"<h5>{node.node_type}</h5>"]

            relation = node.data.get("Relation Name")
            alias = node.data.get("Alias")
            if relation:
                line = f"on {relation}"
                if alias and alias != relation:
                    line += f" {alias}"
                label_lines.append(line + "<br>")

            fields = [
                ("Group Key", "by {}<br>"),
                ("Sort Key", "by {}<br>"),
                ("Join Type", "{} join<br>"),
                ("Index Name", "using {}<br>"),
                ("Hash Cond", "on {}<br>"),
                ("CTE Name", "CTE {}<br>"),
                ("Filter", "where {}<br>"),
                ("Total Cost", "💲 <b>Cost:</b> {}<br>"),
                ("Actual Total Time", "⌛ <b>Time:</b> {}<br>"),
            ]

            for key, template in fields:
                value = node.data.get(key)
                if value:
                    label_lines.append(template.format(value))

            # explanation for node type. all node types can be found in `explain.c` of postgres source code
            explanation_map = {
                "Append": "concatenates the results of multiple subplans. It is typically used to combine results from multiple partitions or UNION ALL queries.",
                "Merge Append": "merges sorted results from multiple subplans into a single sorted output. Useful for ordered queries over partitioned tables.",
                "Nested Loop": "merges two record sets by looping through every record in the first set and trying to find a match in the second set. All matching records are returned.",
                "Merge Join": "merges two record sets by first sorting them on a join key.",
                "Hash Join": "joins two record sets by hashing one of them (using a Hash Scan).",
                "Seq Scan": "finds relevant records by sequentially scanning the input record set. When reading from a table, Seq Scan performs a single read operation (only the table is read).",
                "Gather": "reads the results of the parallel workers, in an undefined order.",
                "Gather Merge": "reads the results of the parallel workers, preserving any ordering.",
                "Index Scan": "finds relevant records based on an index. Index Scans perform 2 read operations: one to read the index and another to read the actual value from the table.",
                "Index Only Scan": "finds relevant records based on an index. Index Only Scan performs a single read operation from the index and does not read from the corresponding table.",
                "Bitmap Index Scan": "uses a Bitmap Index (index which uses 1 bit per page) to find all relevant pages. Results of this node are fed to the Bitmap Heap Scan.",
                "Bitmap Heap Scan": "searches through the pages returned by the Bitmap Index Scan for relevant rows.",
                "Subquery Scan": "executes a subquery and returns the results as a record set. It wraps a subquery used in the FROM clause.",
                "Values Scan": "scans a VALUES clause (i.e., an inline table of literal rows) and returns each row.",
                "CTE Scan": "performs a sequential scan of Common Table Expression (CTE) query results. Note that results of a CTE are materialized (calculated and temporarily stored).",
                "Materialize": "stores the result of its input subplan in memory (or disk if needed) so that it can be scanned multiple times. Often used to break pipelines or avoid recomputation.",
                "Memoize": "is used to cache the results of the inner side of a nested loop. It avoids executing underlying nodes when the results for the current parameters are already in the cache.",
                "Sort": "sorts a record set based on the specified sort key.",
                "Aggregate": "groups records together based on a GROUP BY or aggregate function.",
                "Unique": "removes duplicate rows from the input record set. Often used to implement DISTINCT.",
                "SetOp": "performs set operations such as INTERSECT, INTERSECT ALL, EXCEPT, or EXCEPT ALL on multiple input sets.",
                "Limit": "returns a specified number of rows from a record set.",
                "Hash": "generates a hash table from the records in the input recordset. Hash is used by Hash Join.",
            }
            label_lines.append(
                f"<details><summary>Details</summary><sup><strong>{node.node_type}</strong> {explanation_map.get(node.node_type, "")}</sup></details>")

            label = "".join(label_lines)

            flow_node = StreamlitFlowNode(
                id=node_id,
                # Position will be handled by frontend layout engine if (0,0)
                pos=(0, 0),
                data={"content": label},
                node_type='input' if not parent_id else (
                    'default' if node.children else 'output'),
                source_position='right',
                target_position='left',
                width=300,
                style={'width': '300px'}
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
        qep = self.database.get_qep(query)
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

import json

import matplotlib.pyplot as plt
import networkx as nx
from preprocessing import Database
from project import logger


class QueryPlanNode:
    def __init__(self, node_data: dict) -> None:
        self.node_type = node_data.get("Node Type")
        self.startup_cost = node_data.get("Startup Cost")
        self.total_cost = node_data.get("Total Cost")
        self.children = []
        self.data = node_data  # Store the entire node data for potential future use

    def add_child(self, child_node: "QueryPlanNode") -> None:
        self.children.append(child_node)

    def __str__(self, level: int = 0) -> str:
        ret = "  " * level + repr(self.node_type) + " (Cost: " + str(self.total_cost) + ")\n"
        for child in self.children:
            ret += child.__str__(level + 1)
        return ret


class QueryExecutionPlanGraph:
    def __init__(self, query: str, qep: str) -> None:
        self.query = query
        self.qep = qep
        self.root: QueryPlanNode

        self.execution_graph = nx.DiGraph()

    def _qep_to_json(self) -> dict:
        return json.loads(self.qep)

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

    def generate_nx_graph(self) -> None:
        """
        Generate a NetworkX graph from query

        Args:
            query (str): query string.
        """
        return self._generate_from_pipe_syntax(self.query)

    def _generate_from_pipe_syntax(self, pipe_syntax: str) -> None:
        """
        Generate a NetworkX graph for the execution plan based on the pipe syntax.

        Args:
            pipe_syntax (str): The pipe syntax string.
        """
        # Parse the pipe syntax into steps
        steps = [step.strip() for step in pipe_syntax.split("|>")]

        # Add nodes and edges to the graph
        for i, step in enumerate(steps):
            self.execution_graph.add_node(i, label=step)
            if i > 0:
                self.execution_graph.add_edge(i - 1, i)

    def visualize(self) -> None:
        """
        Visualize the execution plan graph using matplotlib.
        """
        pos = nx.drawing.nx_agraph.graphviz_layout(self.execution_graph, prog="dot", args="-Grankdir=BT")
        labels = nx.get_node_attributes(self.execution_graph, "label")
        nx.draw(self.execution_graph, pos, with_labels=True, labels=labels, node_size=3000, node_color="lightblue")
        plt.show()

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
        # Placeholder for actual implementation
        self.pipesyntax = self._convert_to_pipe_syntax()
        return self.pipesyntax

    def _convert_to_pipe_syntax(self) -> str:
        """
        Convert the qep to pipe syntax.
        """
        # TODO implement the conversion logic
        return """
        FROM customer  
        |>  LEFT  OUTER  JOIN  orders  ON  c_custkey  =  o_custkey  AND  o_comment  NOT  LIKE  
        '%unusual%packages%'  
        |> AGGREGATE COUNT(o_orderkey) c_count GROUP BY c_custkey  
        |> AGGREGATE COUNT(*) AS custdist GROUP BY c_count  
        |> ORDER BY custdist DESC, c_count DESC;
        """

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

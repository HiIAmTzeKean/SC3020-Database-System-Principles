import matplotlib.pyplot as plt
import networkx as nx


class PipeSyntax:
    def __init__(self, query: str) -> None:
        self.query = query
        self.pipesyntax: str
        self.pipesyntax = """
        FROM customer  
        |>  LEFT  OUTER  JOIN  orders  ON  c_custkey  =  o_custkey  AND  o_comment  NOT  LIKE  
        '%unusual%packages%'  
        |> AGGREGATE COUNT(o_orderkey) c_count GROUP BY c_custkey  
        |> AGGREGATE COUNT(*) AS custdist GROUP BY c_count  
        |> ORDER BY custdist DESC, c_count DESC;
        """
    
    def generate(self) -> None:
        """
        Generate the pipe syntax from the query.
        """
        # Placeholder for actual implementation
        self.pipesyntax = """
        FROM customer  
        |>  LEFT  OUTER  JOIN  orders  ON  c_custkey  =  o_custkey  AND  o_comment  NOT  LIKE  
        '%unusual%packages%'  
        |> AGGREGATE COUNT(o_orderkey) c_count GROUP BY c_custkey  
        |> AGGREGATE COUNT(*) AS custdist GROUP BY c_count  
        |> ORDER BY custdist DESC, c_count DESC;
        """

    def __str__(self) -> str:
        return self.pipesyntax
    
    def __repr__(self) -> str:
        return f"PipeSyntax({self.query})"

class QueryExecutionPlan:
    def __init__(self, query: str) -> None:
        self.query = query
        self.execution_plan:str 
        self.execution_graph = nx.DiGraph()
    
    def generate(self, pipe_syntax: str) -> None:
        """
        Generate a NetworkX graph for the execution plan based on the pipe syntax.
        
        Args:
            pipe_syntax (str): The pipe syntax string.
        """
        # Parse the pipe syntax into steps
        steps = [step.strip() for step in pipe_syntax.split('|>')]

        # Add nodes and edges to the graph
        for i, step in enumerate(steps):
            self.execution_graph.add_node(i, label=step)
            if i > 0:
                self.execution_graph.add_edge(i - 1, i)
    
    def visualize(self) -> None:
        """
        Visualize the execution plan graph using matplotlib.
        """
        pos = nx.drawing.nx_agraph.graphviz_layout(self.execution_graph, prog='dot', args='-Grankdir=BT')
        labels = nx.get_node_attributes(self.execution_graph, 'label')
        nx.draw(self.execution_graph, pos, with_labels=True, labels=labels, node_size=3000, node_color="lightblue")
        plt.show()

    def __repr__(self) -> str:
        return f"QueryExecutionPlan({self.query})"
    
class PipeSyntaxParser:
    def __init__(self) -> None:
        pass
    
    def get_pipe_syntax(self, query: str) -> PipeSyntax:
        """
        Pipe syntax given a query string.
        
        Args:
            query (str): The query string to check for pipe syntax.
        
        Returns:
            str: "Pipe" if the query contains pipe syntax, otherwise "No Pipe".
        """
        return PipeSyntax(query)
    
    def get_execution_plan(self, query: str) -> QueryExecutionPlan:
        """
        Get the execution plan for a given query.
        
        Args:
            query (str): The query string to generate the execution plan for.
        
        Returns:
            QueryExecutionPlan: The execution plan object.
        """
        return QueryExecutionPlan(query)
    
    
from dataclasses import dataclass
import json
from collections import namedtuple
from typing import Optional
from streamlit_flow.elements import StreamlitFlowEdge, StreamlitFlowNode

from preprocessing import Database
from project import logger


@dataclass
class FromInfo:
    namespace: Optional[str]
    relation_name: str
    alias: str


@dataclass
class CostInfo:
    startup_cost: float
    total_cost: float
    plan_rows: Optional[float]
    plan_width: Optional[float]


class QueryPlanNode:
    def __init__(self, node_data: dict[str, str | float | list[str]]) -> None:
        self.data = node_data
        self.children: list[QueryPlanNode] = []

        node_type = node_data.get("Node Type")
        join_filter = node_data.get("Join Filter")
        output = node_data.get("Output")
        assert isinstance(node_type, str)
        assert join_filter is None or isinstance(join_filter, str)
        assert isinstance(output, list)
        self.node_type = node_type
        self.join_filter = join_filter
        self.output = output

        if node_type in ["Sample Scan", "Function Scan", "Tid Scan",
                         "Tid Range Scan", "Foreign Scan", "Custom Scan",
                         "WindowAgg", "Modify Table"]:
            # Nodes too complex for our simple prototype.
            logger.error("Unexpected node type: %s", node_type)
            assert False

        self.from_info = None
        if node_type in ["Seq Scan", "Index Scan",
                         "Index Only Scan", "Bitmap Heap Scan"]:
            # Info from ExplainScanTarget.
            relation_name = node_data.get("Relation Name")
            alias = node_data.get("Alias")
            schema = node_data.get("Schema")
            assert isinstance(relation_name, str)
            assert isinstance(alias, str)
            assert schema is None or isinstance(schema, str)
            self.from_info = FromInfo(schema, relation_name, alias)

        self.join_type = None
        if node_type in ["Nested Loop", "Merge Join", "Hash Join"]:
            join_type = node_data.get("Join Type")
            assert isinstance(join_type, str)
            self.join_type = join_type

        startup_cost = node_data.get("Startup Cost")
        if startup_cost is not None:
            total_cost = node_data.get("Total Cost")
            plan_rows = node_data.get("Plan Rows")
            plan_width = node_data.get("Plan Width")
            assert isinstance(startup_cost, float)
            assert isinstance(total_cost, float)
            assert plan_rows is None or isinstance(plan_rows, (float, int))
            assert plan_width is None or isinstance(plan_width, (float, int))
            self.cost_info = CostInfo(
                startup_cost, total_cost, plan_rows, plan_width)

        self.index_or_hash_cond: list[str] = []
        self.order_by = None

        hash_cond = node_data.get("Hash Cond")
        index_cond = node_data.get("Index Cond")
        filter_cond = node_data.get("Filter")
        order_by = node_data.get("Order By")
        sort_key = node_data.get("Sort Key")
        if hash_cond is not None:
            assert isinstance(hash_cond, str)
            self.index_or_hash_cond.append(hash_cond)
        if index_cond is not None:
            assert isinstance(index_cond, str)
            self.index_or_hash_cond.append(index_cond)
        assert filter_cond is None or isinstance(filter_cond, str)
        self.filters = filter_cond
        assert order_by is None or sort_key is None
        assert sort_key is None or isinstance(sort_key, list)
        if order_by is not None:
            assert isinstance(order_by, str)
            self.order_by = [order_by]
        else:
            self.order_by = sort_key

        group_key = node_data.get("Group Key")
        group_set = node_data.get("Grouping Sets")
        assert group_key is None or group_set is None
        assert group_key is None or isinstance(group_key, list)
        assert group_set is None or isinstance(group_set, list)
        self.group_keys: Optional[list[str]] = None
        if group_key is not None:
            self.group_keys = group_key
        elif group_set is not None:
            self.group_keys = group_set

    def full_filter(self, suppress_hash_and_index=False) -> list[str]:
        result: list[str] = []
        if self.filters is not None:
            result.append(self.filters)
        if not suppress_hash_and_index and self.index_or_hash_cond:
            result.extend(self.index_or_hash_cond)
        return result

    def add_child(self, child_node: "QueryPlanNode") -> None:
        self.children.append(child_node)

    def __str__(self, level: int = 0) -> str:
        ret = "|>" * level + repr(self.node_type) + \
            " (Cost: " + str(self.cost_info.total_cost) + ")\n"
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

        def _build_flow_nodes_and_edges(node: QueryPlanNode, parent_id: Optional[str] = None):
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
            if node.from_info:
                label += f"on {node.from_info.relation_name}"
                if node.from_info.relation_name != node.from_info.alias:
                    label += f" {node.from_info.alias}"
                label += "<br>"
            if node.join_type:
                label += f"{node.join_type} join<br>"
            if node.filters:
                label += f"on {node.filters}<br>"

            if node.cost_info:
                label += f"💲 <b>Cost:</b> {node.cost_info.total_cost}<br>"

            if node.data.get("Actual Total Time"):
                label += f"⌛ <b>Time:</b> {
                    node.data.get('Actual Total Time')}<br>"

            label += (
                "<details><summary>Details</summary><sup>more details here if we want</sup></details>"  # TODO details
            )

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


RelationInfo = namedtuple(
    "RelationInfo", ["node_type", "relation_name", "index_condition"])


class PipeSyntax:
    """PipeSyntax class to represent a SQL query in pipe syntax.

    This class holds the SQL query and provides methods to generate the pipe
    syntax.
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
            logger.info("Generating pipe syntax for query: %s", self.query)
            self.qep.generate()
            self.pipesyntax = self._convert_to_pipe_syntax()
        return self.pipesyntax

    def _convert_to_pipe_syntax(self) -> str:
        """
        Convert the qep to pipe syntax.
        """
        piped = self._convert_node_to_pipe_syntax(self.qep.root)
        return "\n|> ".join(" ".join(x) for x in piped)

    def _unwrap_expr(self, expr: list[str]) -> str:
        if len(expr) == 1 and expr[0][0] == '(' and expr[0][-1] == ')':
            return expr[0][1:-1]
        return " AND ".join(expr)

    def _convert_node_to_filter(self, n: QueryPlanNode) -> list[str]:
        if "Bitmap" not in n.node_type:
            logger.error("Unexpected node type in filter: %s", n.node_type)
            assert False
        if n.node_type == "BitmapAnd":
            results = []
            for child in n.children:
                results.extend(self._convert_node_to_filter(child))
            assert len(n.full_filter()) == 0
            return results
        if n.node_type == "BitmapOr":
            results = []
            for child in n.children:
                result = self._convert_node_to_filter(child)
                if len(result) == 1:
                    results.append(result[0])
                else:
                    results.append(f"({" AND ".join(result)})")
            assert len(n.full_filter()) == 0
            return [f"({" OR ".join(results)})"]
        if len(n.full_filter()) > 0:
            return n.full_filter()
        logger.error("Unknown node type in filter: %s", n.node_type)
        assert False

    def _clean_output_list(self, output: list[str]) -> list[str]:
        partial = "PARTIAL "
        cleaned = []
        for x in output:
            if x[0] == "(" and x[-1] == ")":
                x = x[1:-1]
            if x.startswith(partial):
                x = x[len(partial):]
            cleaned.append(x)
        return cleaned

    def _clean_output(self, output: list[str]) -> str:
        return ", ".join(self._clean_output_list(output))

    def _convert_node_to_pipe_syntax(self, n: QueryPlanNode, suppress_hash_index=False) -> list[list[str]]:
        if n.join_type is not None:
            assert len(n.children) == 2
            left = self._convert_node_to_pipe_syntax(n.children[0], True)
            right = self._convert_node_to_pipe_syntax(n.children[1], True)
            # Choose the shorter one as the source.
            if len(left) < len(right):
                left, right = right, left
            right_expr = "\n\t|> ".join(
                " ".join(y.replace("\n", "\n\t") for y in x) for x in right)
            right_text = f"(\n\t{right_expr}\n)"
            if len(right) == 1 and right[0][0] == "FROM":
                # FROM a AS b -> a AS b.
                right_text = " ".join(right[0][1:])
            join_filters = n.full_filter()
            join_filters.extend(n.children[0].index_or_hash_cond)
            join_filters.extend(n.children[1].index_or_hash_cond)
            join_instruction = [
                "JOIN" if n.join_type == "Inner" else f"{n.join_type} JOIN",
                right_text,
                f"ON {self._unwrap_expr(join_filters)}"
            ]
            left.append(join_instruction)
            join_output = set(self._clean_output_list(n.children[0].output))
            join_output.update(self._clean_output_list(n.children[1].output))
            if set(n.output) != join_output:
                left.append(["AGGREGATE", self._clean_output(n.output)])
            return left

        instruction: list[list[str]] = []
        from_group_keys: Optional[list[str]] = []
        filters: list[str] = n.full_filter(suppress_hash_index)
        if n.from_info is not None:
            from_ins = ["FROM", n.from_info.relation_name]
            if n.from_info.alias != n.from_info.relation_name:
                from_ins.extend(["AS", n.from_info.alias])
            instruction = [from_ins]
            from_output = self._clean_output_list(n.output)
            filters = filters.copy()
            filters.extend(f for child in n.children
                           for f in self._convert_node_to_filter(child))
        else:
            assert len(n.children) == 1
            from_output = self._clean_output_list(n.children[0].output)
            from_with_group = n.children[0]
            while not from_group_keys:
                from_group_keys = from_with_group.group_keys
                if not from_with_group.children:
                    break
                from_with_group = from_with_group.children[0]
            instruction = self._convert_node_to_pipe_syntax(n.children[0])

        if filters:
            instruction.append(["WHERE", self._unwrap_expr(filters)])
        if n.order_by:
            instruction.append(["ORDER BY", ", ".join(n.order_by)])
        if ((n.group_keys is not None and n.group_keys != from_group_keys) or
                self._clean_output_list(n.output) != from_output):
            aggregate_instruction = ["AGGREGATE", self._clean_output(n.output)]
            if n.group_keys:
                aggregate_instruction.append("GROUP BY")
                aggregate_instruction.append(", ".join(n.group_keys))
            instruction.append(aggregate_instruction)
        if n.node_type == "Limit":
            limit_row_guess = "???"
            if n.cost_info and n.cost_info.plan_rows:
                limit_row_guess = str(n.cost_info.plan_rows)
            instruction.append(["LIMIT", limit_row_guess])
        return instruction

    def __str__(self) -> str:
        if not hasattr(self, "pipesyntax"):
            self.generate()
        return self.pipesyntax

    def __repr__(self) -> str:
        return f"{self.__class__.__name__}({self.query})"


class PipeSyntaxParser:
    def __init__(self, database: Database) -> None:
        self.database = database
        self.current_query: Optional[str] = None
        self.qep_graph: QueryExecutionPlanGraph
        self.pipesyntax: PipeSyntax

        logger.info("PipeSyntaxParser initialized with database.")

    def _get_qep(self, query: str) -> Optional[str]:
        # with open("sql_qep.txt", "r", encoding="utf-8") as file:
        #     qep = file.read()
        #     return qep
        qep = self.database.get_qep(query)
        return qep

    def _compute_query(self, query: str):
        try:
            qep = self._get_qep(query)
            print(qep)
            assert qep is not None
            self.qep_graph = QueryExecutionPlanGraph(query, qep)
            self.qep_graph.generate()
            self.pipesyntax = PipeSyntax(self.qep_graph)
            self.pipesyntax.generate()
        except AssertionError as e:
            logger.exception("Internal error has occurred.", exc_info=e)

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
        logger.info("Getting pipe syntax for query: %s", query)
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
        logger.info("Getting qep for query: %s", query)
        self._compute_query(query)
        return self.qep_graph


if __name__ == "__main__":
    # Example usage
    db = Database()
    parser = PipeSyntaxParser(db)
    pipe_syntax = parser.get_pipe_syntax("some query")
    print(pipe_syntax)

from pathlib import Path

import pandas as pd
import plotly.graph_objects as go
import dash


def read_and_classify_benchmark_data() -> dict[str, dict[str, pd.DataFrame]]:
    path = Path("performance/results")
    dfs = {"single-thread": {}, "thread-scale": {}}
    for csv in path.glob("*.csv"):
        df = pd.read_csv(csv)
        if "passes" in df.columns and "threads" not in df.columns:
            dfs["single-thread"][csv.stem] = df
        elif "threads" in df.columns and "passes" not in df.columns:
            dfs["thread-scale"][csv.stem] = df
    return dfs


app = dash.Dash(__name__)
benchmark_data = read_and_classify_benchmark_data()

dark_background_color = "#2c2c2c"
dark_text_color = "#84ACCE"
dark_graph_background = "#1c1c1c"
dark_grid_color = "#444444"
dropdown_background_color = "#3b3b3b"
dropdown_hover_background_color = "#4c4c4c"
dropdown_border_color = "#5a5a5a"


def main() -> None:

    elements = [
        [
            dash.html.H1(f"{btype.capitalize().replace('-', ' ')} benchmarks"),
            dash.html.Label("Plot mode"),
            dash.dcc.Dropdown(
                id=f"{btype}-plot-mode",
                options=[
                    {"label": "Lines + markers", "value": "lines+markers"},
                    {"label": "Lines", "value": "lines"},
                    {"label": "Markers", "value": "markers"},
                ],
                value="lines+markers",
                style={
                    "color": dark_text_color,
                },
            ),
            dash.html.Label("Time unit"),
            dash.dcc.Dropdown(
                id=f"{btype}-time-unit",
                options=[
                    {"label": "Nanoseconds", "value": "ns"},
                    {"label": "Microseconds", "value": "us"},
                    {"label": "Milliseconds", "value": "ms"},
                    {"label": "Seconds", "value": "s"},
                ],
                value="s",
            ),
            dash.html.Label("Select axis scale"),
            dash.dcc.Checklist(
                id=f"{btype}-axis-type",
                options=[
                    {"label": "X-Axis logarithmic", "value": "xlog"},
                    {"label": "Y-Axis logarithmic", "value": "ylog"},
                ],
                value=[],
            ),
            dash.html.Label("Select benchmarks"),
            dash.dcc.Checklist(
                id=f"{btype}-checklist",
                options=[
                    {
                        "label": csv_file.capitalize().replace("_", " "),
                        "value": csv_file,
                    }
                    for csv_file in benchmark_data[btype]
                ],
                value=[],
            ),
            dash.dcc.Graph(
                id=f"{btype}-graph",
                style={"padding": "10px", "border": f"1px solid {dark_grid_color}"},
            ),
        ]
        for btype in benchmark_data
    ]
    elements = [element for sublist in elements for element in sublist]

    app.layout = dash.html.Div(
        style={
            "backgroundColor": dark_background_color,
            "color": dark_text_color,
            "padding": "20px",
            "margin": "0",
        },
        children=elements,
    )
    app.run_server(debug=True)


def time_unit_to_factor(time_unit: str) -> int:
    return {
        "ns": 1,
        "us": 1.0 / 1_000,
        "ms": 1.0 / 1_000_000,
        "s": 1.0 / 1_000_000_000,
    }[time_unit]


def update_graph(
    benchmarks: list[str],
    plot_mode: str,
    time_unit: str,
    axis_type: list[str],
    /,
    *,
    benchmark_type: str,
    title: str,
    exclude: str | list[str] | None = None,
) -> go.Figure:
    data = []
    xaxis_type = "log" if "xlog" in axis_type else "linear"
    yaxis_type = "log" if "ylog" in axis_type else "linear"
    xcol = "<unsepcified>"

    factor = time_unit_to_factor(time_unit)
    for benchmark in benchmarks:
        df = benchmark_data[benchmark_type][benchmark]
        if exclude is not None:
            df = df.drop(columns=exclude)

        # Assuming the first column is the x-axis
        xcol = df.columns[0]
        data.extend(
            [
                go.Scatter(
                    x=df[xcol],
                    y=df[col] * factor,
                    mode=plot_mode,
                    marker={"size": 8},
                    name=f"{col.replace('ns', time_unit)} ({benchmark})",
                )
                for col in df.columns[1:]
            ]
        )

    layout = go.Layout(
        title={"text": title, "font": {"color": dark_text_color}},
        plot_bgcolor=dark_graph_background,
        paper_bgcolor=dark_background_color,
        font={"color": dark_text_color},
        xaxis={
            "title": xcol.capitalize(),
            "gridcolor": dark_grid_color,
            "type": xaxis_type,
            "tickmode": "auto",
        },
        yaxis={
            "title": "Time (ns)".replace("ns", time_unit),
            "gridcolor": dark_grid_color,
            "type": yaxis_type,
            "tickmode": "auto",
        },
        showlegend=True,
        legend=dict(font=dict(color=dark_text_color)),
    )
    return go.Figure(data=data, layout=layout)


@app.callback(
    dash.dependencies.Output("single-thread-graph", "figure"),
    [
        dash.dependencies.Input("single-thread-checklist", "value"),
        dash.dependencies.Input("single-thread-plot-mode", "value"),
        dash.dependencies.Input("single-thread-time-unit", "value"),
        dash.dependencies.Input("single-thread-axis-type", "value"),
    ],
)
def update_plain_graph(
    names: list[str], plot_mode: str, time_unit: str, axis_type: list[str], /
) -> go.Figure:
    return update_graph(
        names,
        plot_mode,
        time_unit,
        axis_type,
        benchmark_type="single-thread",
        title="Single thread benchmarks",
    )


@app.callback(
    dash.dependencies.Output("thread-scale-graph", "figure"),
    [
        dash.dependencies.Input("thread-scale-checklist", "value"),
        dash.dependencies.Input("thread-scale-plot-mode", "value"),
        dash.dependencies.Input("thread-scale-time-unit", "value"),
        dash.dependencies.Input("thread-scale-axis-type", "value"),
    ],
)
def update_thread_scale_graph(
    names: list[str], plot_mode: str, time_unit: str, axis_type: list[str], /
) -> go.Figure:
    return update_graph(
        names,
        plot_mode,
        time_unit,
        axis_type,
        benchmark_type="thread-scale",
        title="Thread scale benchmarks",
        exclude="result",
    )


if __name__ == "__main__":
    main()

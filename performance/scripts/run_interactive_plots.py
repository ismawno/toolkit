from pathlib import Path

import pandas as pd
import plotly.graph_objects as go
import dash


def read_csvs() -> dict[str, dict[str, pd.DataFrame]]:
    path = Path("performance/results")
    dfs = {"plain": {}, "per-thread": {}, "thread-scale": {}}
    for csv in path.glob("*.csv"):
        df = pd.read_csv(csv)
        if "passes" in df.columns and "threads" not in df.columns:
            dfs["plain"][csv.stem] = df
        elif "threads" in df.columns and "passes" not in df.columns:
            dfs["thread-scale"][csv.stem] = df
        else:
            for thread in df["threads"].unique():
                df_thread = df[df["threads"] == thread].drop(columns="threads")
                dfs["per-thread"][f"{csv.stem}_threads_{thread}"] = df_thread
    return dfs


app = dash.Dash(__name__)
dfs = read_csvs()
title = "Time (ms)"


def main() -> None:

    elements = [
        [
            dash.html.H3(f"{name.capitalize().replace('-', ' ')} measurements"),
            dash.dcc.Checklist(
                id=f"{name}-checklist",
                options=[
                    {
                        "label": csv_file.capitalize().replace("_", " "),
                        "value": csv_file,
                    }
                    for csv_file in dfs[name]
                ],
                value=[],
            ),
            dash.dcc.Graph(id=f"{name}-graph"),
        ]
        for name in dfs
    ]
    elements = [element for sublist in elements for element in sublist]
    app.layout = dash.html.Div(
        [
            dash.dcc.Checklist(
                id="yaxis-type",
                options=[{"label": "Logarithmic scale", "value": "log"}],
                value=[],
            ),
            *elements,
        ]
    )
    app.run_server(debug=True)


@app.callback(
    dash.dependencies.Output("plain-graph", "figure"),
    [
        dash.dependencies.Input("plain-checklist", "value"),
        dash.dependencies.Input("yaxis-type", "value"),
    ],
)
def update_plain_graph(names: list[str], yaxis_type: list[str]) -> go.Figure:
    if not names:
        return go.Figure()

    yaxis_scale = "log" if "log" in yaxis_type else "linear"

    data = []
    for name in names:
        df = dfs["plain"][name]
        xcol = df.columns[0]
        data.extend(
            [
                go.Scatter(
                    x=df[xcol],
                    y=df[col],
                    mode="markers",
                    marker={"size": 8},
                    name=f"{col} ({name})",
                )
                for col in df.columns[1:]
            ]
        )

    fig = go.Figure(data=data)
    fig.update_layout(
        title="Plain measurements",
        xaxis_title=dfs["plain"][name].columns[0].capitalize(),
        yaxis_title=title,
        yaxis_type=yaxis_scale,
    )
    return fig


@app.callback(
    dash.dependencies.Output("per-thread-graph", "figure"),
    [
        dash.dependencies.Input("per-thread-checklist", "value"),
        dash.dependencies.Input("yaxis-type", "value"),
    ],
)
def update_per_thread_graph(names: list[str], yaxis_type: list[str]) -> go.Figure:
    if not names:
        return go.Figure()

    yaxis_scale = "log" if "log" in yaxis_type else "linear"

    data = []
    for name in names:
        df = dfs["per-thread"][name]
        xcol = df.columns[0]
        data.extend(
            [
                go.Scatter(
                    x=df[xcol],
                    y=df[col],
                    mode="markers",
                    marker={"size": 8},
                    name=f"{col} ({name})",
                )
                for col in df.columns[1:]
            ]
        )

    fig = go.Figure(data=data)
    fig.update_layout(
        title="Per thread measurements",
        xaxis_title=dfs["per-thread"][name].columns[0].capitalize(),
        yaxis_title=title,
        yaxis_type=yaxis_scale,
    )
    return fig


@app.callback(
    dash.dependencies.Output("thread-scale-graph", "figure"),
    [dash.dependencies.Input("thread-scale-checklist", "value")],
)
def update_thread_scale_graph(names: list[str]) -> go.Figure:
    if not names:
        return go.Figure()

    data = []
    for name in names:
        df = dfs["thread-scale"][name]
        xcol = df.columns[0]
        data.extend(
            [
                go.Line(x=df[xcol], y=df[col], name=f"{col} ({name})")
                for col in df.columns[1:]
                if col != "result"
            ]
        )

    fig = go.Figure(data=data)
    fig.update_layout(
        title="Thread scale measurements",
        xaxis_title=dfs["thread-scale"][name].columns[0].capitalize(),
        yaxis_title=title,
    )
    return fig


if __name__ == "__main__":
    main()

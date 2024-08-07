from pathlib import Path

import plotly.graph_objects as go
import pandas as pd


def write_report(df: pd.DataFrame, output_path: Path, md: str) -> str:
    fig = go.Figure(layout_title=f"Time (ms) vs {df.columns[0].capitalize()}")
    md += f"{df.iloc[:, 1:].describe().to_markdown()}\n\n"
    for i in range(1, len(df.columns)):
        mask = df.iloc[:, i] > df.iloc[:, i].quantile(0.8)
        df = df.mask(mask, axis=0)

        fig.add_trace(
            go.Scatter(
                x=df.iloc[:, 0],
                y=df.iloc[:, i],
                mode="markers",
                name=df.columns[i],
                marker=dict(size=3),
            )
        )
    fig.update_layout(
        xaxis_title=df.columns[0].capitalize(),
        yaxis_title="Time (ms)",
    )
    output_path.parent.mkdir(parents=True, exist_ok=True)
    fig.write_image(output_path)
    md += f"![{output_path.stem}]({output_path.name})\n"
    return md


def main() -> None:
    results_folder = Path("performance/results")
    reports_folder = Path("performance/reports")

    reports = {}
    for csv in results_folder.rglob("*.csv"):
        output_folder = reports_folder / csv.parent.relative_to(results_folder)
        md = reports.get(output_folder, "")
        md += "# " + csv.stem.capitalize() + "\n"
        reports[output_folder] = write_report(
            pd.read_csv(csv),
            reports_folder / csv.relative_to(results_folder).with_suffix(".png"),
            md,
        )

    for putput_folder, md in reports.items():
        with open(putput_folder / "index.md", "w") as f:
            f.write(md)


if __name__ == "__main__":
    main()

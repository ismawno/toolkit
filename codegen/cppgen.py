from contextlib import contextmanager
from pathlib import Path


class CPPFile:
    def __init__(self, name: str, /):
        self.__name = name
        self.__file = ""
        self.__scopes = 0

    def __call__(self, line: str, /) -> None:
        tabs = "    " * self.__scopes
        self.__file += f"{tabs}{line}\n"

    @contextmanager
    def scope(
        self,
        name: str | None = None,
        /,
        *,
        semicolon: bool = False,
        indent: bool = True,
        curlies: bool = True,
    ):
        try:
            if name is not None:
                self(name)
            if curlies:
                self("{")
            if indent:
                self.__scopes += 1
            yield
        finally:
            if indent:
                self.__scopes -= 1
            if curlies:
                self("};" if semicolon else "}")

    def write(self, path: Path | None = None, /) -> None:
        if path is None:
            path = Path(self.__name)
        else:
            path = path / self.__name

        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("w") as file:
            file.write(self.__file)

    @property
    def name(self) -> str:
        return self.__name

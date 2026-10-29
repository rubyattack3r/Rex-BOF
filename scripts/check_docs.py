#!/usr/bin/env python3

import pathlib
import re
import urllib.parse


ROOT = pathlib.Path(__file__).resolve().parent.parent
PAGES = [ROOT / "README.md", *sorted((ROOT / "docs").rglob("*.md"))]


def main() -> None:
    missing: list[str] = []
    for page in PAGES:
        text = page.read_text(encoding="utf-8")
        for raw in re.findall(r"!?\[[^]]*\]\(([^)]+)\)", text):
            target = raw.strip("<>").split("#", 1)[0].split("?", 1)[0]
            if not target or "://" in target or target.startswith("mailto:"):
                continue
            resolved = (page.parent / urllib.parse.unquote(target)).resolve()
            if not resolved.exists():
                missing.append(f"{page.relative_to(ROOT)}: {raw}")

    for page in sorted((ROOT / "docs" / "techniques").glob("*.md")):
        if page.name != "index.md" and "```mermaid" not in page.read_text(encoding="utf-8"):
            missing.append(f"{page.relative_to(ROOT)}: missing Mermaid diagram")

    if missing:
        raise SystemExit("documentation checks failed:\n" + "\n".join(missing))
    print(f"checked {len(PAGES)} Markdown files")


if __name__ == "__main__":
    main()


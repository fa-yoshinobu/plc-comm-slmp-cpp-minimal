#!/usr/bin/env python3
"""Generate a Markdown API reference from Doxygen XML."""

from __future__ import annotations

import argparse
import difflib
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from pathlib import Path


MEMBER_ORDER = ("define", "typedef", "enum", "variable", "function", "friend")
COMPOUND_ORDER = ("namespace", "class", "struct", "union")


@dataclass
class EnumValue:
    name: str
    initializer: str
    brief: list[str]


@dataclass
class Member:
    kind: str
    name: str
    signature: str
    brief: list[str] = field(default_factory=list)
    details: list[str] = field(default_factory=list)
    params: list[tuple[str, str]] = field(default_factory=list)
    returns: list[str] = field(default_factory=list)
    enum_values: list[EnumValue] = field(default_factory=list)
    line: int = 0


@dataclass
class Compound:
    kind: str
    name: str
    brief: list[str] = field(default_factory=list)
    details: list[str] = field(default_factory=list)
    members: list[Member] = field(default_factory=list)
    location: str = ""
    line: int = 0


def normalize_space(text: str) -> str:
    return re.sub(r"\s+", " ", text).strip()


def table_cell(text: str) -> str:
    return text.replace("|", "\\|").replace("\n", "<br>")


def inline_text(node: ET.Element | None) -> str:
    if node is None:
        return ""

    pieces: list[str] = []
    if node.text:
        pieces.append(node.text)

    for child in list(node):
        child_text = inline_text(child)
        if child.tag in {"computeroutput", "parametername", "ref"} and child_text:
            pieces.append(f"`{child_text}`")
        elif child.tag == "linebreak":
            pieces.append("\n")
        elif child.tag == "itemizedlist":
            items: list[str] = []
            for item in child.findall("./listitem"):
                item_text = normalize_space(inline_text(item))
                if item_text:
                    items.append(f"- {item_text}")
            if items:
                pieces.append("\n" + "\n".join(items) + "\n")
        elif child_text:
            pieces.append(child_text)

        if child.tail:
            pieces.append(child.tail)

    return "".join(pieces)


def paragraph_texts(node: ET.Element | None) -> list[str]:
    if node is None:
        return []

    result: list[str] = []
    for para in node.findall("./para"):
        text = normalize_space(inline_text(para))
        if text:
            result.append(text)
    return result


def extract_params(node: ET.Element | None) -> list[tuple[str, str]]:
    if node is None:
        return []

    params: list[tuple[str, str]] = []
    for item in node.findall(".//parameterlist[@kind='param']/parameteritem"):
        names = [
            normalize_space(inline_text(name))
            for name in item.findall("./parameternamelist/parametername")
        ]
        names = [name for name in names if name]
        description = " ".join(paragraph_texts(item.find("./parameterdescription")))
        if names or description:
            params.append((", ".join(names), description))
    return params


def extract_returns(node: ET.Element | None) -> list[str]:
    if node is None:
        return []

    returns: list[str] = []
    for simple in node.findall(".//simplesect[@kind='return']"):
        returns.extend(paragraph_texts(simple))
    return returns


def xml_text(parent: ET.Element, tag: str) -> str:
    child = parent.find(tag)
    return normalize_space("".join(child.itertext())) if child is not None else ""


def member_signature(member: ET.Element, kind: str, name: str) -> str:
    definition = xml_text(member, "definition")
    args = xml_text(member, "argsstring")
    initializer = xml_text(member, "initializer")

    if kind == "define":
        return normalize_space(f"#define {name}{args} {initializer}".strip())
    if kind == "function":
        return normalize_space(f"{definition}{args}".strip())
    if kind in {"enum", "typedef", "variable", "friend"}:
        return normalize_space(f"{definition} {initializer}".strip())
    return normalize_space(f"{definition}{args}".strip())


def member_line(member: ET.Element) -> int:
    location = member.find("location")
    if location is None:
        return 0
    try:
        return int(location.get("line", "0"))
    except ValueError:
        return 0


def is_internal_name(name: str) -> bool:
    lowered = name.lower()
    return (
        lowered == "std"
        or "::detail" in lowered
        or lowered.endswith("::detail")
        or "detail::" in lowered
        or "link_direct_detail" in lowered
    )


def should_skip_define(name: str) -> bool:
    if not name:
        return True
    if name.startswith("_") or name.endswith("_H") or name.endswith("_HELPER"):
        return True
    include_guard_suffixes = (
        "_HPP",
        "_INCLUDED",
    )
    return any(name.endswith(suffix) for suffix in include_guard_suffixes)


def parse_member(member: ET.Element) -> Member | None:
    kind = member.get("kind", "")
    prot = member.get("prot", "public")
    name = xml_text(member, "name")
    if prot not in {"public", ""} or not name or is_internal_name(name):
        return None
    if kind == "define" and should_skip_define(name):
        return None
    if kind not in MEMBER_ORDER:
        return None

    details_node = member.find("detaileddescription")
    enum_values: list[EnumValue] = []
    if kind == "enum":
        for value in member.findall("enumvalue"):
            value_name = xml_text(value, "name")
            if not value_name:
                continue
            enum_values.append(
                EnumValue(
                    name=value_name,
                    initializer=xml_text(value, "initializer"),
                    brief=paragraph_texts(value.find("briefdescription")),
                )
            )

    return Member(
        kind=kind,
        name=name,
        signature=member_signature(member, kind, name),
        brief=paragraph_texts(member.find("briefdescription")),
        details=paragraph_texts(details_node),
        params=extract_params(details_node),
        returns=extract_returns(details_node),
        enum_values=enum_values,
        line=member_line(member),
    )


def parse_compound(path: Path) -> Compound | None:
    root = ET.parse(path).getroot()
    compound_node = root.find("compounddef")
    if compound_node is None:
        return None

    kind = compound_node.get("kind", "")
    if kind not in COMPOUND_ORDER and kind != "file":
        return None

    name = xml_text(compound_node, "compoundname")
    if is_internal_name(name):
        return None

    location = compound_node.find("location")
    file_path = ""
    line = 0
    if location is not None:
        file_path = location.get("file", "")
        try:
            line = int(location.get("line", "0"))
        except ValueError:
            line = 0

    members: list[Member] = []
    for section in compound_node.findall("sectiondef"):
        section_kind = section.get("kind", "")
        if kind in {"class", "struct", "union"} and not section_kind.startswith("public"):
            continue
        for member_node in section.findall("memberdef"):
            parsed = parse_member(member_node)
            if parsed is not None:
                members.append(parsed)

    if kind == "file":
        members = [member for member in members if member.kind == "define"]
        if not members:
            return None

    if kind != "file" and not members and not paragraph_texts(compound_node.find("briefdescription")):
        return None

    members.sort(key=lambda item: (item.line, item.kind, item.name))
    return Compound(
        kind=kind,
        name=name,
        brief=paragraph_texts(compound_node.find("briefdescription")),
        details=paragraph_texts(compound_node.find("detaileddescription")),
        members=members,
        location=file_path,
        line=line,
    )


def doxy_quote(path: Path | str) -> str:
    return '"' + str(path).replace("\\", "/").replace('"', '\\"') + '"'


def run_doxygen(
    *,
    title: str,
    root: Path,
    inputs: list[Path],
    predefined: list[str],
) -> list[Compound]:
    if shutil.which("doxygen") is None:
        raise SystemExit("doxygen was not found on PATH")

    with tempfile.TemporaryDirectory(prefix="cpp_api_ref_") as tmp_name:
        tmp = Path(tmp_name)
        output_dir = tmp / "doxygen"
        doxyfile = tmp / "Doxyfile"
        input_paths = " ".join(doxy_quote(path.resolve()) for path in inputs)
        predefined_values = " ".join(predefined)
        doxyfile.write_text(
            "\n".join(
                [
                    f"PROJECT_NAME = {title}",
                    f"OUTPUT_DIRECTORY = {doxy_quote(output_dir)}",
                    f"INPUT = {input_paths}",
                    "GENERATE_HTML = NO",
                    "GENERATE_LATEX = NO",
                    "GENERATE_XML = YES",
                    "XML_PROGRAMLISTING = NO",
                    "RECURSIVE = NO",
                    "EXTRACT_ALL = YES",
                    "EXTRACT_PRIVATE = NO",
                    "EXTRACT_STATIC = YES",
                    "HIDE_UNDOC_MEMBERS = NO",
                    "HIDE_UNDOC_CLASSES = NO",
                    "SORT_MEMBER_DOCS = NO",
                    "SORT_BRIEF_DOCS = NO",
                    "QUIET = YES",
                    "WARN_IF_UNDOCUMENTED = NO",
                    "WARN_IF_DOC_ERROR = YES",
                    "WARN_NO_PARAMDOC = NO",
                    "WARN_AS_ERROR = NO",
                    "BUILTIN_STL_SUPPORT = YES",
                    "MARKDOWN_SUPPORT = YES",
                    "AUTOLINK_SUPPORT = YES",
                    "ENABLE_PREPROCESSING = YES",
                    "MACRO_EXPANSION = NO",
                    f"PREDEFINED = {predefined_values}",
                    "EXCLUDE_SYMBOLS = *::detail *detail::* *link_direct_detail*",
                    "EXTENSION_MAPPING = h=C++ hpp=C++",
                    "",
                ]
            ),
            encoding="utf-8",
        )

        subprocess.run(["doxygen", str(doxyfile)], cwd=root, check=True)
        xml_dir = output_dir / "xml"
        compounds = [
            compound
            for path in sorted(xml_dir.glob("*.xml"))
            if path.name != "index.xml"
            for compound in [parse_compound(path)]
            if compound is not None
        ]
        compounds.sort(key=lambda item: (item.kind != "file", item.location, item.line, item.name))
        return compounds


def render_member(member: Member, *, class_member: bool) -> list[str]:
    lines = [f"#### `{member.name}`", ""]
    if member.signature:
        lines.extend(["```cpp", member.signature, "```", ""])

    paragraphs = member.brief + member.details
    for paragraph in paragraphs:
        lines.extend([paragraph, ""])

    if member.params:
        lines.extend(["| Parameter | Description |", "| --- | --- |"])
        for name, description in member.params:
            lines.append(f"| `{table_cell(name)}` | {table_cell(description)} |")
        lines.append("")

    if member.returns:
        lines.append("Returns: " + " ".join(member.returns))
        lines.append("")

    if member.enum_values:
        lines.extend(["| Value | Description |", "| --- | --- |"])
        for value in member.enum_values:
            initializer = f" {value.initializer}" if value.initializer else ""
            description = " ".join(value.brief)
            lines.append(f"| `{table_cell(value.name + initializer)}` | {table_cell(description)} |")
        lines.append("")

    return lines


def member_group_title(kind: str, *, class_member: bool) -> str:
    if kind == "define":
        return "Configuration Macros"
    if kind == "typedef":
        return "Aliases"
    if kind == "enum":
        return "Enums"
    if kind == "variable":
        return "Fields" if class_member else "Variables And Constants"
    if kind == "function":
        return "Member Functions" if class_member else "Functions"
    if kind == "friend":
        return "Friends"
    return kind.title()


def render_compound(compound: Compound) -> list[str]:
    title_kind = {
        "namespace": "Namespace",
        "class": "Class",
        "struct": "Struct",
        "union": "Union",
        "file": "File Macros",
    }.get(compound.kind, compound.kind.title())
    title = (
        f"Public Header Macros In `{compound.location}`"
        if compound.kind == "file"
        else f"{title_kind} `{compound.name}`"
    )
    lines = [f"### {title}", ""]

    if compound.kind != "file":
        for paragraph in compound.brief + compound.details:
            lines.extend([paragraph, ""])

    grouped: dict[str, list[Member]] = {kind: [] for kind in MEMBER_ORDER}
    for member in compound.members:
        grouped.setdefault(member.kind, []).append(member)

    class_member = compound.kind in {"class", "struct", "union"}
    for kind in MEMBER_ORDER:
        members = grouped.get(kind, [])
        if not members:
            continue
        lines.extend([f"#### {member_group_title(kind, class_member=class_member)}", ""])
        for member in members:
            lines.extend(render_member(member, class_member=class_member))

    return lines


def render_markdown(title: str, inputs: list[Path], compounds: list[Compound]) -> str:
    lines: list[str] = [
        f"# {title}",
        "",
        "This file is generated from Doxygen XML for the public C++ headers.",
        "Do not edit it manually; run `scripts/generate_api_reference.py` instead.",
        "",
        "## Header Inputs",
        "",
    ]

    for path in inputs:
        lines.append(f"- `{path.as_posix()}`")
    lines.append("")

    file_compounds = [compound for compound in compounds if compound.kind == "file"]
    for compound in file_compounds:
        lines.extend(render_compound(compound))

    for kind in COMPOUND_ORDER:
        group = [compound for compound in compounds if compound.kind == kind]
        if not group:
            continue
        group_title = {
            "namespace": "Namespaces",
            "class": "Classes",
            "struct": "Structs",
            "union": "Unions",
        }[kind]
        lines.extend([f"## {group_title}", ""])
        for compound in group:
            lines.extend(render_compound(compound))

    return "\n".join(lines).rstrip() + "\n"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--title", required=True)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--input", action="append", required=True, dest="inputs", type=Path)
    parser.add_argument("--predefine", action="append", default=[])
    parser.add_argument("--check", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = Path.cwd()
    inputs = [path if path.is_absolute() else root / path for path in args.inputs]
    missing = [path for path in inputs if not path.exists()]
    if missing:
        for path in missing:
            print(f"[ERROR] input not found: {path}", file=sys.stderr)
        return 2

    compounds = run_doxygen(
        title=args.title,
        root=root,
        inputs=inputs,
        predefined=args.predefine,
    )
    relative_inputs = [path.relative_to(root) if path.is_absolute() else path for path in inputs]
    generated = render_markdown(args.title, relative_inputs, compounds)
    output = args.output if args.output.is_absolute() else root / args.output

    if args.check:
        current = output.read_text(encoding="utf-8") if output.exists() else ""
        if current != generated:
            diff = difflib.unified_diff(
                current.splitlines(),
                generated.splitlines(),
                fromfile=str(output),
                tofile="generated",
                lineterm="",
            )
            print("[ERROR] API reference is out of date. Run scripts/generate_api_reference.py.", file=sys.stderr)
            for index, line in enumerate(diff):
                if index >= 120:
                    print("... diff truncated ...", file=sys.stderr)
                    break
                print(line, file=sys.stderr)
            return 1
        print(f"[OK] {args.output} is up to date.")
        return 0

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(generated, encoding="utf-8", newline="\n")
    print(f"Generated {args.output} from {len(compounds)} Doxygen compounds.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

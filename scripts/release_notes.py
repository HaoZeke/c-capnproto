#!/usr/bin/env python3
"""Print the CHANGELOG.md section for one version.

Exit non-zero when that section is missing. Link reference lines at the
bottom of a section are dropped so the text can be used as release notes.
"""
import argparse
import re
import sys


def section(text, version):
    head = re.search(
        r"^## \[" + re.escape(version) + r"\] - .*?$",
        text,
        re.M,
    )
    if not head:
        sys.exit("CHANGELOG.md has no section for %s" % version)
    rest = text[head.end():]
    nxt = re.search(r"^## \[", rest, re.M)
    body = rest[: nxt.start()] if nxt else rest
    lines = []
    for line in body.strip().splitlines():
        if re.match(r"^\[[^\]]+\]: https?://", line):
            continue
        lines.append(line)
    return "\n".join(lines).strip() + "\n"


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("version", help="version without a leading v, such as 0.4.2")
    parser.add_argument("-o", "--output", help="write the section here")
    args = parser.parse_args(argv)
    with open("CHANGELOG.md", encoding="utf-8") as fh:
        text = fh.read()
    body = section(text, args.version)
    if args.output:
        with open(args.output, "w", encoding="utf-8") as out:
            out.write(body)
    else:
        sys.stdout.write(body)


if __name__ == "__main__":
    main(sys.argv[1:])

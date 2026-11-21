#!/bin/sh
set -eu
export LC_ALL=C

if [ "$#" -lt 2 ]; then
  printf '%s\n' 'usage: check-dyn-export.sh <library> <header-or-dir>...' >&2
  exit 1
fi

library=$1
shift

n=$#
while [ "$n" -gt 0 ]; do
  arg=$1
  shift
  n=$((n - 1))
  if [ -d "$arg" ]; then
    for h in "$arg"/*.h; do
      [ -f "$h" ] || continue
      case "$h" in
        */capnp_priv.h|capnp_priv.h) continue ;;
      esac
      set -- "$@" "$h"
    done
  else
    case "$arg" in
      */capnp_priv.h|capnp_priv.h) continue ;;
    esac
    set -- "$@" "$arg"
  fi
done

if [ "$#" -eq 0 ]; then
  printf '%s\n' 'no public headers given' >&2
  exit 1
fi

tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

status=0
if ! awk '
  function strip_comments(line, out, bs, ls, e, rest) {
    out = ""
    while (line != "") {
      if (in_comment) {
        e = index(line, "*/")
        if (!e)
          return out
        line = substr(line, e + 2)
        in_comment = 0
        continue
      }
      bs = index(line, "/*")
      ls = index(line, "//")
      if (ls && (!bs || ls < bs))
        return out substr(line, 1, ls - 1)
      if (!bs)
        return out line
      out = out substr(line, 1, bs - 1) " "
      rest = substr(line, bs + 2)
      e = index(rest, "*/")
      if (!e) {
        in_comment = 1
        return out
      }
      line = substr(rest, e + 2)
    }
    return out
  }
  function check_decl(d, pre, name, k) {
    gsub(/[ \t\r\n]+/, " ", d)
    sub(/^ /, "", d)
    sub(/ $/, "", d)
    if (d == "" || d ~ /^(typedef|extern[ ]+"C")([ ]|$)/)
      return
    if (d ~ /(^|[ ])(CAPN_INLINE|static)([ ]|$)/)
      return
    k = index(d, "(")
    if (!k)
      return
    pre = substr(d, 1, k - 1)
    sub(/[ ]+$/, "", pre)
    if (!match(pre, /[A-Za-z_][A-Za-z0-9_]*$/))
      return
    name = substr(pre, RSTART, RLENGTH)
    print name
    if (d !~ /(^|[ ])CAPN_EXPORT([ ]|$)/) {
      print "public declaration lacks CAPN_EXPORT: " d > "/dev/stderr"
      failed = 1
    }
  }
  FNR == 1 {
    in_comment = 0
    in_pp = 0
    depth = 0
    decl = ""
  }
  in_pp {
    in_pp = ($0 ~ /\\$/)
    next
  }
  !in_comment && /^[ \t]*#/ {
    in_pp = ($0 ~ /\\$/)
    next
  }
  {
    line = strip_comments($0)
    n = length(line)
    for (i = 1; i <= n; i++) {
      ch = substr(line, i, 1)
      if (ch == "{") {
        if (depth == 0 && decl ~ /^[ \t]*extern[ \t]+"C"[ \t]*$/) {
          decl = ""
        } else {
          depth++
          decl = ""
        }
      } else if (ch == "}") {
        if (depth > 0)
          depth--
        decl = ""
      } else if (ch == ";" && depth == 0) {
        check_decl(decl)
        decl = ""
      } else if (depth == 0) {
        decl = decl ch
      }
    }
    if (depth == 0)
      decl = decl " "
  }
  END {
    exit failed
  }
' "$@" > "$tmpdir/raw_expected"; then
  status=1
fi

sort -u "$tmpdir/raw_expected" > "$tmpdir/expected"
if [ ! -s "$tmpdir/expected" ]; then
  printf '%s\n' 'no public declarations found' >&2
  exit 1
fi

case "$library" in
  *.a|*.lib)
    if [ "$status" -ne 0 ]; then
      exit "$status"
    fi
    printf '%s\n' "static library $library; skipping dynamic export check"
    exit 77
    ;;
  *.dylib)
    if ! command -v nm >/dev/null 2>&1; then
      if [ "$status" -ne 0 ]; then
        exit "$status"
      fi
      printf '%s\n' 'nm not found; skipping dynamic export check'
      exit 77
    fi
    if ! nm -g "$library" > "$tmpdir/nm_out"; then
      printf '%s\n' "nm -g failed on $library" >&2
      exit 1
    fi
    awk 'NF == 3 && $2 ~ /^[A-Z]$/ && $2 != "U" {
      sub(/^_/, "", $3)
      print $3
    }' "$tmpdir/nm_out" > "$tmpdir/raw_actual"
    ;;
  *.so|*.so.*)
    if ! command -v nm >/dev/null 2>&1; then
      if [ "$status" -ne 0 ]; then
        exit "$status"
      fi
      printf '%s\n' 'nm not found; skipping dynamic export check'
      exit 77
    fi
    if ! nm -D --defined-only "$library" > "$tmpdir/nm_out"; then
      printf '%s\n' "nm -D --defined-only failed on $library" >&2
      exit 1
    fi
    awk 'NF >= 3 && $2 ~ /^[A-Z]$/ && $2 != "U" {
      sub(/@.*/, "", $3)
      print $3
    }' "$tmpdir/nm_out" > "$tmpdir/raw_actual"
    ;;
  *)
    if [ "$status" -ne 0 ]; then
      exit "$status"
    fi
    printf '%s\n' "unsupported library $library; skipping dynamic export check"
    exit 77
    ;;
esac

sort -u "$tmpdir/raw_actual" > "$tmpdir/actual"
if ! comm -23 "$tmpdir/expected" "$tmpdir/actual" > "$tmpdir/missing"; then
  printf '%s\n' 'could not compare declarations with library symbols' >&2
  exit 1
fi

if [ -s "$tmpdir/missing" ]; then
  printf '%s\n' 'public symbols missing from shared library:' >&2
  cat "$tmpdir/missing" >&2
  status=1
fi

exit "$status"

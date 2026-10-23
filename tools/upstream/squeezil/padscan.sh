#!/bin/bash
# padscan.sh targets.txt "src1.cpp src2.cpp ..." [N list]  -- the compiler's register choices depend on how many symbols the
# translation unit has declared (and where). This writes N dummy declarations into every source and prints
# exact-count / total-score / non-zero scores for each combination, so a near-miss unit can be nudged to 0 by adding the right
# number of (plausible) declarations.
#
# Dimensions (space-separated lists in the environment; defaults reproduce the old padscan):
#   PAD_KINDS  proto   void PadDecl001();
#              fwd     class PadFwd001;
#              extern  extern int g_pad001;
#              shape   class PadShape001 { public: void m1() {} ... };   (PAD_M unused inline members, default 4)
#   PAD_ATS    top     first lines of the file
#              inc     right after the last #include line
#              eof     end of the file (still moves code above it: isle/ReproBit uses this "suffix" placement)
# Example: PAD_KINDS="proto fwd extern shape" PAD_ATS="top inc eof" padscan.sh targets.txt unit.cpp "1 2 3 4 6 8"
#
# The sources are edited IN PLACE (backup under .padscan_backup/, restored on exit), because the compiler sees the full source
# path and isle found the path spelling changes code; the old "__pad_<name>.cpp" copy changed the file name.
# PAD_DRY=1 only shows where the declarations land in the first source (no compile).
# If a run is killed hard, the backup stays and the next run refuses to start until you restore it.
HERE="$(cd "$(dirname "$0")" && pwd)"
T="$1"; SRCS="$2"; NS="${3:-0 1 2 3 4 5 6 8 10 12}"
KINDS="${PAD_KINDS:-proto}"; ATS="${PAD_ATS:-top}"; M="${PAD_M:-4}"
BK="$HERE/.padscan_backup"
if [ -d "$BK" ] && [ -n "$(ls -A "$BK")" ]; then
  echo "padscan: $BK is not empty: an earlier run did not restore its sources. Copy them back (see $BK/MAP), then delete $BK."; exit 1
fi
mkdir -p "$BK"; : > "$BK/MAP"
ABS=(); i=0
for s in $SRCS; do
  case "$s" in /*) a="$s";; *) a="$(pwd)/$s";; esac
  [ -f "$a" ] || { echo "padscan: no such file $a"; rm -rf "$BK"; exit 1; }
  cp -p "$a" "$BK/$i"; echo "$i $a" >> "$BK/MAP"; ABS+=("$a"); i=$((i+1))
done
restore() { local j=0; for a in "${ABS[@]}"; do cp -p "$BK/$j" "$a"; j=$((j+1)); done; rm -rf "$BK"; }
trap restore EXIT; trap 'exit 130' INT TERM

gen() { # kind n
  local k=$1 n=$2 j f
  for j in $(seq 1 "$n"); do
    j=$(printf "%03d" "$j")
    case "$k" in
      proto)  echo "void PadDecl$j();";;
      fwd)    echo "class PadFwd$j;";;
      extern) echo "extern int g_pad$j;";;
      shape)  printf "class PadShape%s { public:" "$j"; for f in $(seq 1 "$M"); do printf " void m%d() {}" "$f"; done; echo " };";;
      *) echo "padscan: unknown kind $k" >&2; exit 1;;
    esac
  done
}
place() { # at padfile orig > out
  case "$1" in
    top) cat "$2" "$3";;
    eof) cat "$3"; echo; cat "$2";;
    inc) awk -v pf="$2" 'NR==FNR { if ($0 ~ /^[ \t]*#[ \t]*include/) last=FNR; next }
           { print } FNR==last { while ((getline l < pf) > 0) print l }
           END { if (!last) { while ((getline l < pf) > 0) print l } }' "$3" "$3";;
    *) echo "padscan: unknown placement $1" >&2; exit 1;;
  esac
}
run() { # label
  local out ex tot
  out=$("$HERE/match.sh" "$T" "${ABS[@]}" 2>&1 | grep -E "^[A-Za-z0-9_]+=[0-9]+$")
  ex=$(echo "$out" | grep -c "=0$"); tot=$(echo "$out" | awk -F= '{s+=$2} END{print s+0}')
  echo "$1 exact=$ex sum=$tot $(echo "$out" | grep -v '=0$' | tr '\n' ' ')"
}

PF="$BK/pad.txt"
[ -z "$PAD_DRY" ] && for n in $NS; do [ "$n" = 0 ] && { run "baseline N=0"; break; }; done
for k in $KINDS; do for at in $ATS; do for n in $NS; do
  [ "$n" = 0 ] && continue
  gen "$k" "$n" > "$PF" || exit 1
  j=0; for a in "${ABS[@]}"; do place "$at" "$PF" "$BK/$j" > "$a" || exit 1; j=$((j+1)); done
  if [ -n "$PAD_DRY" ]; then echo "=== $k@$at N=$n: ${ABS[0]}"; grep -n -C1 -E "Pad|g_pad" "${ABS[0]}"; continue; fi
  run "$k@$at N=$n"
done; done; done

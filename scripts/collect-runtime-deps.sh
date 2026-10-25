#! /bin/bash
#
# Prints the distribution packages that provide the shared libraries the given
# binaries need, one package name per line.
#
# The amuleapi curl-smoke job runs binaries that were built in another job, on
# a machine that has no compiler and no development packages. It therefore has
# to install the runtime libraries alone. Naming them by hand would drift the
# moment aMule grows a dependency, and installing the build job's whole -dev
# list instead drags in every header set and development package for a job
# that compiles nothing -- which is what made that job miss its timeout when
# the package mirror was slow.
#
# So the list is derived from the binaries themselves: ldd names the shared
# objects a binary loads, and dpkg-query names the packages that ship them.
# The build job runs this on the binaries it is about to ship, so the list
# cannot fall out of step with them.
#
# Usage: scripts/collect-runtime-deps.sh BINARY [BINARY ...]

set -u

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 BINARY [BINARY ...]" >&2
    exit 2
fi

packages=""
status=0

for binary in "$@"; do
    if [ ! -x "$binary" ]; then
        echo "$0: $binary is not an executable" >&2
        exit 2
    fi

    # ldd prints one "soname => /path (0x...)" line per shared object, plus a
    # line for the dynamic loader and one for the vDSO, neither of which is a
    # file on disk. A library that cannot be resolved is reported as
    # "soname => not found", which is an error here rather than something to
    # skip: the smoke job would fail to start the binary.
    while IFS= read -r library; do
        case "$library" in
            "!unresolved "*)
                echo "$0: $binary needs ${library#!unresolved }, which is not installed" >&2
                status=1
                continue
                ;;
        esac

        package="$(dpkg-query -S "$library" 2>/dev/null | head -n 1 | cut -d: -f1)"
        if [ -z "$package" ]; then
            echo "$0: no package provides $library" >&2
            status=1
            continue
        fi

        packages="$packages $package"
    done < <(ldd "$binary" 2>/dev/null | awk '$2 == "=>" { if ($3 ~ /^\//) print $3; else print "!unresolved " $1 }')
done

# A library nobody provides would leave the smoke job with a binary that
# cannot start, so a partial list is a failed list.
if [ "$status" -ne 0 ]; then
    exit 1
fi

[ -n "$packages" ] || exit 0

# Unquoted on purpose: this is a list of package names to split on whitespace.
printf '%s\n' $packages | sort -u

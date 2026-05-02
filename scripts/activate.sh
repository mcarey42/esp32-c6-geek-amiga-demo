# Source this file to load ESP-IDF v6.0 into the current shell.
#     . scripts/activate.sh
#
# We deliberately bypass the vendor activation script's sourcing path.
# Two v6.0 quirks break it for our use:
#   1. It defines `idf.py` as a shell alias, which doesn't expand in
#      non-interactive `bash -c` or shebanged scripts.
#   2. Its is_sourced() check inspects ${0##*/} against a bash/sh/ksh
#      whitelist, so sourcing activate.sh from `./scripts/build-hw.sh`
#      fails (because $0 = build-hw.sh) and the vendor script exits 1.
#
# Instead we invoke the vendor script with `-e`, which prints the
# resolved environment-variable assignments, export them, then define
# idf.py / esptool.py / espefuse.py as real shell functions.

_V6_ACTIVATE="$HOME/.espressif/tools/activate_idf_v6.0.sh"
if [[ ! -f "$_V6_ACTIVATE" ]]; then
    echo "error: $_V6_ACTIVATE not found — install ESP-IDF v6.0 via eim first" >&2
    return 1 2>/dev/null || exit 1
fi

_v6_saved_path="$PATH"
while IFS= read -r _v6_line; do
    [[ -n "$_v6_line" ]] && export "$_v6_line"
done < <(bash "$_V6_ACTIVATE" -e)
# -e prints PATH as just the IDF-tools segment. When sourced normally
# the vendor script does `PATH=tools:$PATH`, so re-append our saved PATH.
export PATH="$PATH:$_v6_saved_path"
unset _v6_line _v6_saved_path

idf.py()    { "$IDF_PYTHON_ENV_PATH/bin/python" "$IDF_PATH/tools/idf.py" "$@"; }
esptool.py()    { "$IDF_PYTHON_ENV_PATH/bin/python" "$IDF_PATH/components/esptool_py/esptool/esptool.py" "$@"; }
espefuse.py()   { "$IDF_PYTHON_ENV_PATH/bin/python" "$IDF_PATH/components/esptool_py/esptool/espefuse.py" "$@"; }

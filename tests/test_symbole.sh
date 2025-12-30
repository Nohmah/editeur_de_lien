#!/bin/bash

ELF_READER=../elf_reader
READELF=arm-none-eabi-readelf
EXAMPLES_DIR=../Examples_fusion

RED='\033[48;5;1m'
GREEN='\033[48;5;2m'
NC='\033[0m'
ERREUR=0

check() {
    local label="$1"
    local got="$2"
    local exp="$3"

    if [ "$got" = "$exp" ]; then
        echo -e "  $label : ${GREEN}OK${NC}"
    else
        echo -e "  $label : ${RED}FAIL${NC} ($got ≠ $exp)"
        ERREUR=1
    fi
}

echo "=== TEST TABLE DES SYMBOLES (-s) ==="
echo

for f in "$EXAMPLES_DIR"/*.o; do
    [ -f "$f" ] || continue
    name=$(basename "$f")

    echo "Test : $name"

    out=$("$ELF_READER" "$f" -s)
    expected=$("$READELF" -s "$f")

    mapfile -t sym_out < <(echo "$out" | grep -E '^[[:space:]]*[0-9]+')
    mapfile -t sym_exp < <(echo "$expected" | grep -E '^[[:space:]]*[0-9]+:')

    count=${#sym_exp[@]}

    for ((i=0; i<count; i++)); do
        l_out="${sym_out[$i]}"
        l_exp="${sym_exp[$i]}"

        idx=$(echo "$l_out" | awk '{print $1}')
        echo " Symbole [$idx]"

        # Value
        check "Value" \
            "$(echo "$l_out" | awk '{print $2}')" \
            "$(echo "$l_exp" | awk '{print $2}')"

        # Type
        check "Type" \
            "$(echo "$l_out" | awk '{print $3}')" \
            "$(echo "$l_exp" | awk '{print $4}')"

        # Bind
        check "Bind" \
            "$(echo "$l_out" | awk '{print $4}')" \
            "$(echo "$l_exp" | awk '{print $5}')"

        # Section
        check "Section" \
            "$(echo "$l_out" | awk '{print $5}')" \
            "$(echo "$l_exp" | awk '{print $7}')"

        # Name
        check "Name" \
            "$(echo "$l_out" | awk '{print $6}')" \
            "$(echo "$l_exp" | awk '{print $8}')"

        echo
    done
done

exit $ERREUR

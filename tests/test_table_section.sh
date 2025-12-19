#!/bin/bash

ELF_READER=./elf_reader
READELF=arm-none-eabi-readelf
EXAMPLES_DIR=Examples_fusion

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

echo "=== TEST TABLE DES SECTIONS (-S) ==="
echo

for f in "$EXAMPLES_DIR"/exemple*.o; do
    [ -f "$f" ] || continue
    name=$(basename "$f")

    echo "Test : $name"

    out=$("$ELF_READER" "$f" -S)
    #-W option added to not allow truncation
    expected=$("$READELF" -S -W "$f")

    # Retrieve the section index from the arm-none-eabi-readelf command.
    mapfile -t idxs < <(echo "$expected" | grep -E '^\s*\[[[:space:]]*[0-9]+\]' | sed -E 's/^\s*\[\s*([0-9]+)\].*/\1/')

    for idx in "${idxs[@]}"; do
        #The sed part remove the [Nr] field.
        line_out=$(echo "$out" | grep -E "^\[\s*${idx}\]" | head -1 | sed -E 's/^\s*\[\s*[0-9]+\]\s*//')
        line_exp=$(echo "$expected" | grep -E "^\s*\[\s*${idx}\]" | head -1 | sed -E 's/^\s*\[\s*[0-9]+\]\s*//')

        name_out=$(echo "$line_out" | awk '{print $1}')

        echo " Section [$idx] $name_out"
    
        #Name
        check "Name" \
        "$name_out" \
        "$(echo "$line_exp" | awk '{print $1}')"

        #Type
        check "Type" \
        "$(echo "$line_out" | awk '{print $2}')" \
        "$(echo "$line_exp" | awk '{print $2}')"

        #Address
        check "Addr" \
        "$(echo "$line_out" | awk '{print $3}')" \
        "$(echo "$line_exp" | awk '{print $3}')"

        #Offset
        check "Off" \
        "$(echo "$line_out" | awk '{print $4}')" \
        "$(echo "$line_exp" | awk '{print $4}')"

        #Size
        check "Size" \
        "$(echo "$line_out" | awk '{print $5}')" \
        "$(echo "$line_exp" | awk '{print $5}')"

        #Es
        check "Es" \
        "$(echo "$line_out" | awk '{print $6}')" \
        "$(echo "$line_exp" | awk '{print $6}')"
        
        #Flags
        check "Flags" \
        "$(echo "$line_out" | awk '{print $7}')" \
        "$(echo "$line_exp" | awk '{print $7}')"

        #Link
        check "Link" \
        "$(echo "$line_out" | awk '{print $8}')" \
        "$(echo "$line_exp" | awk '{print $8}')"

        #Info
        check "Info" \
        "$(echo "$line_out" | awk '{print $9}')" \
        "$(echo "$line_exp" | awk '{print $9}')"

        #Align
        check "Align" \
        "$(echo "$line_out" | awk '{print $10}')" \
        "$(echo "$line_exp" | awk '{print $10}')"

        echo
    done
done

exit $ERREUR
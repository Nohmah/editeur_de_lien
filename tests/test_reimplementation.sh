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
        echo -en "  $label : ${GREEN}OK${NC}"
    else
        echo -en "  $label : ${RED}FAIL${NC} ($got ≠ $exp)"
        ERREUR=1
    fi
    echo -n " "
}

echo "=== TEST REIMPLANTATION (-r) ==="

for f in "$EXAMPLES_DIR"/*.o; do
    [ -f "$f" ] || continue
    name=$(basename "$f")
    echo -e "\n"
    echo "Test : $name"

    out=$("$ELF_READER" "$f" -r)
    expected=$("$READELF" -r "$f")

    # Test entêtes de sections
    mapfile -t sec_out < <(echo "$out" | grep 'Section de readressage')
    mapfile -t sec_exp < <(echo "$expected" | grep 'Relocation section')
    
    sections_out=${#sec_out[@]}
    sections_exp=${#sec_exp[@]}
    
    check "Sections" "$sections_out" "$sections_exp"
    echo
    
    for ((s=0; s<sections_out && s<sections_exp; s++)); do
        sec_name_out=$(echo "${sec_out[$s]}" | sed -n "s/.*readressage \.\([^ ]*\).*/\1/p")
        sec_offset_out=$(echo "${sec_out[$s]}" | sed -n "s/.*offset \(0x[0-9a-f]*\).*/\1/p")
        sec_count_out=$(echo "${sec_out[$s]}" | sed -n "s/.*contient \([0-9]*\).*/\1/p")
        
        sec_name_exp=$(echo "${sec_exp[$s]}" | sed -n "s/.*section '\.\([^']*\)'.*/\1/p")
        sec_offset_exp=$(echo "${sec_exp[$s]}" | sed -n "s/.*offset \(0x[0-9a-f]*\).*/\1/p")
        sec_count_exp=$(echo "${sec_exp[$s]}" | sed -n "s/.*contains \([0-9]*\).*/\1/p")
        
        echo -n "Section .$sec_name_out: "
        check "offset" "$sec_offset_out" "$sec_offset_exp"
        check "count" "$sec_count_out" "$sec_count_exp"
        echo
    done

    mapfile -t reloc_out < <(echo "$out" | grep -E '^[[:space:]]*[0-9a-f]{8}[[:space:]]')
    mapfile -t reloc_exp < <(echo "$expected" | grep -E '^[[:space:]]*[0-9a-f]{8}[:]?[[:space:]]')

    count_out=${#reloc_out[@]}
    count_exp=${#reloc_exp[@]}

    if [ "$count_out" -gt 0 ]; then
        
        for ((i=0; i<count_out; i++)); do
            echo -n "Réimplantation $((i+1)):"
            
            out_line="${reloc_out[$i]}"
            exp_line="${reloc_exp[$i]}"
            
            # Offset
            out_offset=$(echo "$out_line" | awk '{print $1}')
            exp_offset=$(echo "$exp_line" | awk '{print $1}' | sed 's/://')
            check "Offset" "$out_offset" "$exp_offset"
            
            # Info
            out_info=$(echo "$out_line" | awk '{print $2}')
            exp_info=$(echo "$exp_line" | awk '{print $2}')
            check "Info" "$out_info" "$exp_info"
            
            # Type
            out_type=$(echo "$out_line" | awk '{print $3}')
            exp_type=$(echo "$exp_line" | awk '{print $3}')
            check "Type" "$out_type" "$exp_type"
            
            # Sym.Value
            out_symval=$(echo "$out_line" | awk '{print $4}')
            exp_symval=$(echo "$exp_line" | awk '{print $4}')
            check "Sym.Value" "$out_symval" "$exp_symval"
            
            # Sym.Name
            out_symname=$(echo "$out_line" | sed -E 's/^([^[:space:]]+[[:space:]]+){3}//' | sed 's/  \+/ /g')
            exp_symname=$(echo "$exp_line" | sed -E 's/^([^[:space:]]+[[:space:]]+){3}//' | sed 's/  \+/ /g')
            check "Sym.Name" "$out_symname" "$exp_symname"
            
        done
    fi
done
echo
exit $ERREUR
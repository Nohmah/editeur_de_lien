#!/bin/bash

ELF_READER=./elf_reader
READELF=arm-none-eabi-readelf
EXAMPLES_DIR=Examples_fusion

RED='\033[48;5;1m'
GREEN='\033[48;5;2m'
NC='\033[0m'

normalize_hex() {
    printf "0x%x" "$(( $1 ))"
}

check() {
    local label="$1"
    local got="$2"
    local exp="$3"

    if [ "$got" = "$exp" ]; then
        echo -e "  $label : ${GREEN}OK${NC}"
    else
        echo -e "  $label : ${RED}FAIL${NC} ($got ≠ $exp)"
    fi
}

echo "=== TEST EN-TÊTE ELF (-h) ==="
echo

for f in "$EXAMPLES_DIR"/exemple*.o; do
    [ -f "$f" ] || continue
    name=$(basename "$f")

    echo "Test : $name"

    out=$("$ELF_READER" "$f" -h)
    expected=$("$READELF" -h "$f")

    # Magique
    check "Magic" \
    "$(echo "$out"  | grep "Magic:" | sed 's/^[^:]*://;s/  */ /g;s/^ //')" \
    "$(echo "$expected" | grep "Magic:"   | sed 's/^[^:]*://;s/  */ /g;s/^ //')"


    # Classe
    check "Class" \
        "$(echo "$out" | grep "Class:" | awk '{print $2}')" \
        "$(echo "$expected" | grep "Class:" | awk '{print $2}')"

    # Données
    check "Data" \
        "$(echo "$out" | grep "Data:" | grep -o "Big Endian")" \
        "Big Endian"

    # Version
    check "Version" \
        "$(echo "$out" | grep "Version:" | head -1 | awk '{print $2}')" \
        "1"

    # OS/ABI
    check "OS/ABI" \
        "$(echo "$out" | grep "OS/ABI:" | awk '{print $2}')" \
        "0"

    # Version ABI
    check "ABI Version" \
        "$(echo "$out" | grep "ABI Version:" | awk '{print $3}')" \
        "$(echo "$expected" | grep "ABI Version:" | grep -o '[0-9]\+')"

    # Type
    check "Type" \
        "$(echo "$out" | grep "Type:" | awk '{print $2}')" \
        "REL"

    # Machine
    check "Machine" \
        "$(echo "$out" | grep "Machine:" | awk '{print $2}')" \
        "ARM"

    # Adresse point d'entrée
    out_entry=$(normalize_hex "$(echo "$out" | grep "Entry point address:" | awk '{print $NF}')")
    expected_entry=$(normalize_hex "$(echo "$expected" | grep "Entry point address:" | awk '{print $NF}')")
    check "Entry point" "$out_entry" "$expected_entry"

    # Début en-têtes de programme
    check "PH offset" \
        "$(echo "$out" | grep "Start of program headers:" | grep -o '[0-9]\+')" \
        "$(echo "$expected" | grep "Start of program headers:" | grep -o '[0-9]\+')"

    # Début en-têtes de section
    check "SH offset" \
        "$(echo "$out" | grep "Start of section headers:" | grep -o '[0-9]\+')" \
        "$(echo "$expected" | grep "Start of section headers:" | grep -o '[0-9]\+')"

    # Flags
    out_flags=$(normalize_hex "$(echo "$out" | grep "Flags:" | awk '{print $2}')")
    expected_flags=$(normalize_hex "$(echo "$expected" | grep "Flags:" | awk '{print $2}' | tr -d ',')")
    check "Flags" "$out_flags" "$expected_flags"

    # Taille en-tête
    check "EH size" \
        "$(echo "$out" | grep "Size of this header:" | grep -o '[0-9]\+')" \
        "$(echo "$expected" | grep "Size of this header:" | grep -o '[0-9]\+')"

    # Taille en-tête programme
    check "PH entry size" \
        "$(echo "$out" | grep "Size of program headers:" | grep -o '[0-9]\+')" \
        "$(echo "$expected" | grep "Size of program headers:" | grep -o '[0-9]\+')"

    # Nombre d'en-tête programme
    check "PH number" \
        "$(echo "$out" | grep "Number of program headers:" | grep -o '[0-9]\+')" \
        "$(echo "$expected" | grep "Number of program headers:" | grep -o '[0-9]\+')"

    # Taille en-tête section
    check "SH entry size" \
        "$(echo "$out" | grep "Size of section headers:" | grep -o '[0-9]\+')" \
        "$(echo "$expected" | grep "Size of section headers:" | grep -o '[0-9]\+')"

    # Nombre en-tête section
    check "SH number" \
        "$(echo "$out" | grep "Number of section headers:" | grep -o '[0-9]\+')" \
        "$(echo "$expected" | grep "Number of section headers:" | grep -o '[0-9]\+')"

    # Table index chaîne en-tête section
    check "SH string index" \
        "$(echo "$out" | grep "Section header string table index:" | grep -o '[0-9]\+')" \
        "$(echo "$expected" | grep "Section header string table index:" | grep -o '[0-9]\+')"

    echo
done

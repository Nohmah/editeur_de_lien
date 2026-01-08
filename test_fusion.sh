#!/bin/bash

# Couleurs pour l'affichage
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # Pas de couleur

echo "=========================================="
echo "  Script de Test - Fusion ELF (Étape 8)"
echo "=========================================="
echo ""

# Vérifier si elf_reader existe
if [ ! -f "../elf_reader" ]; then
    echo -e "${RED}Erreur: elf_reader n'existe pas${NC}"
    echo "Compilez d'abord avec: make"
    exit 1
fi

# Chercher les fichiers .o dans le répertoire courant
echo "Recherche de fichiers .o dans le répertoire..."
OBJ_FILES=($(ls *.o 2>/dev/null | grep -v "elf_.*\.o" | grep -v "fusion_.*\.o" | grep -v "fonc_.*\.o"))

if [ ${#OBJ_FILES[@]} -lt 2 ]; then
    echo -e "${YELLOW}Pas assez de fichiers .o trouvés (besoin de 2 minimum)${NC}"
    echo ""
    echo "Génération de fichiers de test..."
    
    # Créer des fichiers assembleur simples pour les tests
    cat > test1.s << 'EOF2'
    .text
    .global func1
func1:
    push {lr}
    bl func2
    pop {pc}

    .data
data1:
    .word 0x12345678
EOF2

    cat > test2.s << 'EOF2'
    .text
    .global func2
func2:
    push {lr}
    bl func1
    pop {pc}

    .data
data2:
    .word 0x87654321
EOF2

    # Compiler les fichiers de test
    echo "Compilation de test1.s et test2.s..."
    arm-none-eabi-as -mbig-endian -o test1.o test1.s
    arm-none-eabi-as -mbig-endian -o test2.o test2.s
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Erreur: arm-none-eabi-as n'est pas installé${NC}"
        echo "Installez-le avec: sudo apt-get install gcc-arm-none-eabi"
        exit 1
    fi
    
    OBJ_FILES=(test1.o test2.o)
    echo -e "${GREEN}Fichiers de test créés: test1.o test2.o${NC}"
fi

FILE1=${OBJ_FILES[0]}
FILE2=${OBJ_FILES[1]}

echo ""
echo "=========================================="
echo "Fichiers à fusionner:"
echo "  Fichier 1: $FILE1"
echo "  Fichier 2: $FILE2"
echo "=========================================="
echo ""

# Test 1: Affichage de l'en-tête
echo -e "${YELLOW}[Test 1] Affichage de l'en-tête de $FILE1${NC}"
../elf_reader $FILE1 -h
echo ""

# Test 2: Affichage des sections
echo -e "${YELLOW}[Test 2] Affichage des sections de $FILE1${NC}"
../elf_reader $FILE1 -S
echo ""

# Test 3: Affichage de la table des symboles
echo -e "${YELLOW}[Test 3] Affichage de la table des symboles de $FILE1${NC}"
../elf_reader $FILE1 -s
echo ""

# Test 4: Affichage des réimplantations
echo -e "${YELLOW}[Test 4] Affichage des réimplantations de $FILE1${NC}"
../elf_reader $FILE1 -r
echo ""

# Test 5: Fusion complète avec réimplantations (Étape 8)
echo -e "${YELLOW}[Test 5] Fusion complète avec réimplantations (Étape 8)${NC}"
../elf_reader $FILE1 -F $FILE2
echo ""

# Vérifier si la fusion a réussi
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Test de fusion réussi!${NC}"
    
    # Comparer avec les outils GNU si disponibles
    if command -v arm-none-eabi-ld &> /dev/null; then
        echo ""
        echo -e "${YELLOW}[Comparaison] Création d'une référence avec GNU ld${NC}"
        arm-none-eabi-ld -r -EB -o reference.o $FILE1 $FILE2 2>/dev/null
        
        if [ -f reference.o ]; then
            echo "Référence créée: reference.o"
            echo ""
            echo "Pour comparer les relocations:"
            echo "  arm-none-eabi-readelf -r reference.o"
            echo ""
            echo "Vous pouvez maintenant comparer manuellement les sorties"
        fi
    fi
else
    echo -e "${RED}✗ Échec de la fusion${NC}"
    exit 1
fi

echo ""
echo "=========================================="
echo "  Tests terminés"
echo "=========================================="

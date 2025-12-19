#!/bin/bash

# Tests pour la partie 3: comparaison entre readelf -x et elf_reader -x

DIR="Examples_fusion"
PROGRAM="./elf_reader"

# Vérification que les fichiers existent
if [ ! -f "$PROGRAM" ] || [ ! -d "$DIR" ]; then
    echo "❌ ERREUR: Fichier ou dossier manquant(s)"
    exit 1
fi

ERREUR=0

for ELF_FILE in "$DIR"/*.o; do	
	if [ ! -f "$ELF_FILE" ]; then
        echo "❌ ERREUR: Aucun fichier .o trouvé dans $DIR"
        exit 1
    fi
    NOM_FICHIER=$(basename "$ELF_FILE")

	# Récupérer la liste des sections
	SECTIONS=$(readelf -S "$ELF_FILE" | awk '$2 ~ /^\./ {print $2}')

	for SECTION in $SECTIONS; do
		# Fichiers temporaires
		SORTIE_TEST=$($PROGRAM "$ELF_FILE" -x "$SECTION" 2>/dev/null)
		SORTIE_ATTENDU=$(readelf -x "$SECTION" "$ELF_FILE" 2>/dev/null)

		# Vérification section vide
		if echo "$SORTIE_TEST" | grep -q "Section vide" && echo "$SORTIE_ATTENDU" | grep -q "no data to dump"; then
		    printf "%-30s %-30s %s\n" "FICHIER $NOM_FICHIER" "SECTION $SECTION" "correcte"
		    continue
		fi

		# Comparaison des données si non vide
		DATA_TEST=$(echo "$SORTIE_TEST" | grep -E '^[[:space:]]*0x[0-9a-f]+')
		DATA_ATTENDU=$(echo "$SORTIE_ATTENDU" | grep -E '^[[:space:]]*0x[0-9a-f]+')

		# Résultat final
		if [ "$DATA_TEST" = "$DATA_ATTENDU" ]; then
		    printf "%-30s %-30s %s\n" "FICHIER $NOM_FICHIER" "SECTION $SECTION" "correcte"
		else
		    printf "%-30s %-30s %s\n" "FICHIER $NOM_FICHIER" "SECTION $SECTION" "INCORRECT"
		    ERREUR=1
		fi
	done
done

exit $ERREUR
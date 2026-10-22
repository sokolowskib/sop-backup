#!/bin/bash

# === KONFIGURACJA ===
SRC="source"
DST="backup"
DELAY=1.5  # Czas (sekundy) między operacjami

# Kolory dla czytelności
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# 0. Wstępne czyszczenie i przygotowanie
echo -e "${BLUE}=== START TESTU (Source: $SRC -> Dest: $DST) ===${NC}"
echo -e "Czyszczenie katalogu źródłowego..."

# Usuwamy stare źródło i tworzymy nowe, czyste
if [ -d "$SRC" ]; then rm -rf "$SRC"; fi
mkdir -p "$SRC"

echo -e "${BLUE}Upewnij się, że w swoim programie dodałeś: add $SRC $DST${NC}"
echo "Start za 3 sekundy..."
sleep 3

# 1. Tworzenie plików (IN_CREATE, IN_CLOSE_WRITE)
echo -e "${GREEN}[1/7] Tworzenie plików...${NC}"
echo "Wersja 1" > "$SRC/plik_tekstowy.txt"
touch "$SRC/pusty_plik.log"
sleep $DELAY

# 2. Tworzenie katalogów zagnieżdżonych (IN_CREATE | IN_ISDIR)
echo -e "${GREEN}[2/7] Tworzenie struktury katalogów...${NC}"
mkdir -p "$SRC/poziom1/poziom2"
echo "Dane w podkatalogu" > "$SRC/poziom1/poziom2/gleboki.data"
sleep $DELAY

# 3. Modyfikacja treści (IN_MODIFY / IN_CLOSE_WRITE)
echo -e "${GREEN}[3/7] Modyfikacja pliku...${NC}"
echo "Dopisana linijka numer 2" >> "$SRC/plik_tekstowy.txt"
sleep $DELAY

# 4. Zmiana atrybutów/uprawnień (IN_ATTRIB)
echo -e "${GREEN}[4/7] Zmiana uprawnień (chmod)...${NC}"
chmod 777 "$SRC/pusty_plik.log"
chmod 700 "$SRC/poziom1"
sleep $DELAY

# 5. Przenoszenie / Zmiana nazwy (IN_MOVED_FROM -> IN_MOVED_TO)
echo -e "${GREEN}[5/7] Przenoszenie i zmiana nazwy...${NC}"
mv "$SRC/plik_tekstowy.txt" "$SRC/plik_zmieniony.txt"
mv "$SRC/poziom1/poziom2/gleboki.data" "$SRC/poziom1/przeniesiony.data"
sleep $DELAY

# 6. Usuwanie plików (IN_DELETE)
echo -e "${RED}[6/7] Usuwanie plików...${NC}"
rm "$SRC/plik_zmieniony.txt"
sleep $DELAY

# 7. Usuwanie katalogów (IN_DELETE | IN_ISDIR)
echo -e "${RED}[7/7] Usuwanie całego katalogu (rm -rf)...${NC}"
rm -rf "$SRC/poziom1"
sleep $DELAY

echo -e "${BLUE}=== KONIEC OPERACJI ===${NC}"
echo "Weryfikacja zawartości..."

# === WERYFIKACJA AUTOMATYCZNA ===
if [ ! -d "$DST" ]; then
    echo -e "${RED}BŁĄD KRYTYCZNY: Katalog docelowy '$DST' nie istnieje!${NC}"
    echo "Czy na pewno uruchomiłeś program: ./sop-backup add $SRC $DST ?"
    exit 1
fi

# Używamy diff -r żeby porównać całą strukturę
DIFF_OUT=$(diff -r "$SRC" "$DST")

if [ -z "$DIFF_OUT" ]; then
    echo -e "${GREEN}SUKCES! Katalogi '$SRC' i '$DST' są identyczne!${NC}"
else
    echo -e "${RED}PORAŻKA! Wykryto różnice:${NC}"
    echo "$DIFF_OUT"
    echo -e "\n${RED}Interpretacja błędów:${NC}"
    echo "- 'Only in $SRC': Worker nie skopiował pliku."
    echo "- 'Only in $DST': Worker nie usunął pliku (błąd IN_DELETE)."
    echo "- 'binary files differ': Treść się nie zaktualizowała (błąd IN_MODIFY)."
fi

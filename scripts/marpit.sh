#!/bin/bash
# marpit.sh: Build Marp talks to HTML and generate index
set -e
# Change to the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

# Create public directory if not present
mkdir -p public
grep -v '^#' .marp-talks | while IFS= read -r file; do
  echo "seen $file"
done


# Build Marp talks to HTML
files=()
while IFS= read -r line; do
  files+=("$line")
done < <(grep -v '^#' .marp-talks)  # Extract lines to an array

for file in "${files[@]}"; do
  file="$(echo "$file" | xargs)"  # Trim whitespace at the beginning and end of the line
  echo "Processing $file"
  [ -z "$file" ] && continue
  # Vérification que le fichier existe
  if [ ! -f "$file" ]; then
    echo "File $file does not exist. Skipping."
    continue
  fi
  # Relative path without extension
  relpath="${file%.md}"
  # Source directory (where the .md is)
  srcdir=$(dirname "$file")
  # Target directory in public/
  dstdir="public/$srcdir"
  mkdir -p "$dstdir"
  # Copy all contents from the source directory (files and subdirectories)
  if [ -d "$srcdir" ]; then
    cp -r "$srcdir"/* "$dstdir"/ 2>/dev/null || true
  fi
  # Generate the HTML in the target directory by running marp in a separate shell
  out="$(basename "$file" .md).html"
  marp --html --allow-local-files "$file" -o "$dstdir/$out"
  #echo "Running marp in a separate shell for $file"
  #bash -c "cd \"$dstdir\" && marp \"$file\" --html --allow-local-files -o \"$out\""
  # Check if HTML file was generated and report its size
  htmlpath="$dstdir/$out"
  if [ -f "$htmlpath" ]; then
    size=$(stat -c %s "$htmlpath" 2>/dev/null || stat -f %z "$htmlpath" 2>/dev/null)
    echo "Generated $htmlpath ($size bytes)"
  else
    echo "File $htmlpath was not generated."
  fi
done

echo "All talks processed."
# Add index.html listing all talks
echo '<!DOCTYPE html><html><head><meta charset="utf-8"><title>Daniel Lemire&#39;s talks</title><link rel="stylesheet" href="styles.css"></head><body><h1>Daniel Lemire&#39;s talks</h1>' > public/index.html
# Collect all talks and years
awk 'BEGIN{FS="/"} /^[^#]/ && NF>0 { year=$1; talks[year]=talks[year] $0 "\n"; years[year]=1 } END { for (y in years) print y }' .marp-talks | sort -r > years.txt
while read year; do
  [ -z "$year" ] && continue
  echo "<h2>$year</h2><ul>" >> public/index.html
  grep "^$year/" .marp-talks | while read file; do
    [ -z "$file" ] && continue
    case "$file" in \#*) continue ;; esac
    relpath="${file%.md}"
    htmlpath="$relpath.html"
    basename=$(basename "$htmlpath")
    echo "<li><a href='$htmlpath'>$basename</a></li>" >> public/index.html
  done
  echo "</ul>" >> public/index.html
done < years.txt
rm years.txt
echo '</body></html>' >> public/index.html
cp css/styles.css public/ || true

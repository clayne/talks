# Marp Talks to GitHub Pages CI

This workflow builds specified Marp Markdown files into HTML and deploys them to GitHub Pages.

## Usage

1. List your Marp Markdown files in `.marp-talks` (one per line, relative to repo root).
2. Push to `master` (or change the branch below).
3. The workflow will build and publish the HTML to GitHub Pages.

## Requirements
- Each Markdown file must be a valid Marp slide deck (YAML frontmatter with `marp: true`).
- Output HTML will be placed in `public/` and published to GitHub Pages.

---

Example `.marp-talks` file:
```
2022/evil/week1/week1.md
2018/megadata.md
```

---

## Customization
- Edit `.marp-talks` to add/remove talks.
- Edit the workflow file for advanced options (theme, output dir, etc).

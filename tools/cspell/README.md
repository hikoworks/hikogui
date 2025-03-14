# CSpell Configuration

You are in the CSpell configuration folder.

It contains the following files:

- The file `cspell.config.json` is the cspell configuration file.
  It is included through the `cspell.json` file in the project's root folder.

- The `repo-words.txt` file contains a list of allowed words related to this
  repository, preventing them from triggering spell checker warnings.

## Github Actions Integration

CSpell is integrated into the project using a GitHub Actions workflow
defined in `.github/workflows/check-spelling.yml`.

This workflow performs the following steps:

1. Uses npm to install the CSpell spell checker.
2. Runs a spell check on the repository.
3. If no spelling errors are found, the workflow completes successfully.
4. If spelling errors are detected, the workflow fails, providing feedback on the errors.

Spelling errors are checked during the CI/CD process.
The workflow triggers on every push and can be run manually.

## How to Use

1. **Install CSpell Extension**:
    - Install the CSpell extension for your code editor (e.g., VSCode).

2. **Fix Spelling Errors**:
    - If a word is underlined, you can either:
      - Correct the spelling manually.
      - Use the suggestion provided by the spell checker.

3. **Add Words to Repository**:
    - If the word is correct but not recognized by the spell checker:
      - Right-click on the underlined word.
      - Select "Spelling > Add Words to Workspace Dictionary" from the context menu.

## Links

- [CSpell Project](https://cspell.org/)
- [VSCode Extension: streetsidesoftware.code-spell-checker](https://marketplace.visualstudio.com/items?itemName=streetsidesoftware.code-spell-checker)
  - [GitHub Repository](https://github.com/streetsidesoftware/vscode-spell-checker)

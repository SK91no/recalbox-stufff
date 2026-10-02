# How to contribute to the Recalbox project

1. Create a Merge Request with your changes.
   Every single fix/feature is developed in its own branch and associated with a Merge Request.
2. Check if the **linters** of the pipeline succeed.
   The linters can be first executed locally by running `./run_linters` from the root of the project.
   The linting process consists of checking Buildroot files for syntax error, deprecated features, ...
   It also checks configurations files.
   And finally, it tests your commit messages that must follow Conventional Commits (https://www.conventionalcommits.org/en/v1.0.0/)
3. Push your branch and rebase against master as frequently as possible.
4. Propose a merge request
   The merge request is labelled with ~Testing::Beta label, by the Recalbox team. This label indicates to embed in the next beta.
4. Once tested by the team, the merge request can be merged (fast-foward only, so rebasing often is important).
   Do not forget to insert an entry in the `RELEASE-NOTES.md`.

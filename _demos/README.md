# _demos

Source for small interactive demos embedded in blog posts. Not published (leading underscore); only build output under `assets/demos/` is.

- One folder per post, named after it: `_demos/<yyyy-mm-slug>/`
- Output goes to `assets/demos/<yyyy-mm-slug>/` and is committed
- `sh _demos/build.sh` rebuilds everything

Bigger things that grow their own CMake, tests or CI move to their own repo.

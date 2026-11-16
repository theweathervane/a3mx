# Running the tests in Docker

A container that clones the code from a git server (by default the project's GitLab,
`https://gitlab.com/theweathervane-group/a3mx`, or any mirror you point it at), extracts **your** disc, builds the `headless`
preset and runs every `ctest` test ([docs/TESTS.md](../../docs/TESTS.md)). Nothing from the disc is
in the image: the ISO is mounted read-only when the container runs.

| File | |
|---|---|
| `Dockerfile` | Ubuntu 24.04 with the packages of CI's `build:linux-x64` job |
| `run-tests.sh` | the entry point: clone / fetch, extract (once), build, test, write the results |
| `docker-compose.yml` | the same as a Portainer stack |
| `run.sh` | build and run from a shell on the Docker host |

## With Portainer

1. On the Docker host, make two folders:
   - one with your ULUS-10062 `.iso` in it, mounted read-only at `/iso`. Other discs can be there
     too: the right one is picked by checksum;
   - one for the tests, mounted at `/tests`: the clone and build go in `a3mx/`, each run's results in
     `results/`.
2. **Stacks > Add stack > Repository**: the repository URL (GitLab, or the mirror you use),
   reference `refs/heads/main`, compose path `docker/tests/docker-compose.yml`. Add authentication if
   the repository is private.
3. **Environment variables**: `A3MX_ISO_DIR` and `A3MX_TESTS_DIR`, the two folders (both required), and
   optionally `A3MX_GIT_URL` (to test a mirror, such as a Gitea on your own network), `A3MX_TESTS`,
   `A3MX_JOBS` (below).
4. **Deploy**. The container runs once and stops; its log is the test output. To test again (the
   newest code is fetched each time), start the container again (**Containers > a3mx-tests > Start**).
   Redeploy the stack only when this folder changes.

## From a shell

```
./run.sh ISO_FOLDER TESTS_FOLDER            # everything
./run.sh ISO_FOLDER TESTS_FOLDER -e A3MX_TESTS="boot_to_select|arcade_fight"
```

Settings you always want (a mirror's address, a token) can go in `docker/tests/.env`, one
`NAME=value` per line: `run.sh` and `docker compose` both read it, and it is git-ignored, so nothing
about your own network ends up in the repository.

## Settings

| Variable | Default | |
|---|---|---|
| `A3MX_GIT_URL` | `https://gitlab.com/theweathervane-group/a3mx.git` | where to clone from: any mirror works |
| `A3MX_GIT_REF` | `main` | branch, tag or commit |
| `A3MX_GIT_TOKEN` | | an access token, for a private repository (sent as a header, never stored) |
| `A3MX_GIT_USER` | `oauth2` | the user name sent with the token: GitLab accepts `oauth2`; on Gitea, your user name |
| `A3MX_ISO` | | one ISO, as a path inside the container (`/iso/...`). By default every `.iso` in the folder is offered and `extract_assets.py` picks ULUS-10062 by its program's checksum, so other discs can sit beside it |
| `A3MX_SUITE` | `disc` | `disc`: the headless build's tests (parity and the rest); `ci`: only the disc-free ones, as GitLab CI; `both` |
| `A3MX_TESTS` / `A3MX_EXCLUDE` | | `ctest -R` / `-E` regexes |
| `A3MX_JOBS` | every core | parallel jobs. The tests are independent; about 2½ h at 2 jobs on a 2-core machine |
| `A3MX_REEXTRACT` | `0` | `1`: extract the disc again (after changing the analysis tools) |
| `A3MX_CLEAN` | `0` | `1`: delete the build folders first |

## What it keeps and writes

- `TESTS_FOLDER/a3mx/` holds the clone, `work/`, `assets/us/` and `build-headless/`, so a later run
  only fetches, rebuilds what changed and tests. The disc is extracted again only when the ISO it
  came from changes or goes (adding other ISOs to the folder doesn't), or with `A3MX_REEXTRACT=1`. To start over, delete that folder. The container runs as root, so its files there belong
  to root.
- Each run writes `TESTS_FOLDER/results/<date-time>/` (`results/latest` points to the newest): `run.log`, `summary.txt`,
  `commit.txt`, `junit-headless.xml`, `LastTest-headless.log` and `failed-headless.txt`. The exit code is
  0 when every test passed.
- Everything derived from the disc stays in `TESTS_FOLDER/a3mx/` and must never be committed or pushed.

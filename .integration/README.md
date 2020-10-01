## Continuous integration

Continuous integration (CI) tools, is a pratice for building code source on different architectures (Linux, OS X, Window) and running unit tests for each git commit or GitHub pull requests. This allows:
* to detect code regressions (running unit tests or static code analysis).
* to detect compilation regressions by compiling with multiple compilers and compiler versions.
* to compile the project on several different architectures.

We use the following tools:
* [Travis-CI](https://travis-ci.org/) for launching compilation on Linux and OS X (file `../.travis.yml`).
* [Appveyor](https://www.appveyor.com/) for launching compilation on Windows (file `../appveyor.yml`).
* [Coverity Scan](https://scan.coverity.com/) for performing static code analysis.
* [Coveralls](https://coveralls.io/) for displaying code coverage.

Coverity Scan is called manually:
```
cd .. && make coverity-scan
```

Travis-CI and Appveyor are triggered on each git commit or GitHub pull request. On success, Travis-CI will trigger Coveralls.

__Note:__ Travis-CI only uses outdated Ubuntu version which. Therefore, a Docker may be used with Travis-CI with latest Ubuntu version.

## Files

In this directory, here the description of the following files (grouped as theme):

##### Travis-CI

* travis-install-linux.sh: bash script called by the Travis-CI ../.travis.yml file for installing Ubuntu packages, needed for the project.
* travis-install-osx.sh: bash script called by the Travis-CI ../.travis.yml file for installing OSX packages needed by the project.
* travis-launch_tests.sh: bash script called by the Travis-CI ../.travis.yml file for launching the project compilation and running unit tests (for Linux and OSX).
* travis-deploy.sh: if travis-launch_tests.sh success: perform some specific deployment tasks like calling coveralls (can also launch OBS).

##### Appveyor

appveyor-install-window.sh: bash script called by the Appveyor ../appveyor.yml file for installing Windows packages needed by the project.
appveyor-launch_tests.sh: bash script called by the Travis-CI ../appveyor.yml file for launching the project compilation and running unit tests for Windows.

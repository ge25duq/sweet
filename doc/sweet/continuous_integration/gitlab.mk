# Gitlab CI

We use a continuous integration (CI) to run various tests under various environments.
This targets to improve the compatibility across different systems and typically also leads to various other benefits such as better coding style, less error-prone code, etc.

## SWEET's Gitlab CI

The CI system we're using is Gitlab CI and is hosted at INRIA.

The corresponding URL is

```https://gitlab.inria.fr/mschreib/sweet-ci-tests```

Since SWEET is still hosted at github.com, we need to do a few workarounds.



## Setting up CI test cases

Because SWEET is hosted at github.com, new CI test cases or a change of them have to be set up manually at INRIA Gitlab.

This is done by calling ```mule.ci_gitlab.setup_ci_tests.py``` (from ```mule_local/bin```).

Based on the content in ```./tests``` it generates the corresponding ```./.gitlab-ci.yml``` file in SWEET's root directory.

The content of this file has to be copied manually to the INRIA Gitlab CI (ask Martin to do this!) into the CI Editor.



## Starting CI test cases

The test cases can be also manually triggered by calling the following command in a shell:

```curl -X POST --fail -F token=glptt-0d66598d696a02da33fb65e2a039f607c68ea50d -F ref=main https://gitlab.inria.fr/api/v4/projects/42631/trigger/pipeline```

Please do so only if you really want to start a test.

Running these tests is very compute intensive. Think about our environment, green computing and the CO2 footprint!



## Further reading
Further information basically just to setup the CI system
```
https://inria-ci.gitlabpages.inria.fr/doc/page/gitlab/
```

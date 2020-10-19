
## Contribution

Any contribution like raise issues after testing, feature discussion, code review, documentment enhancement and packaging for more platforms are very much welcomed.

### Discuss the bug and feature before PR

Currently, there is no user forum for this project, so github issues, <https://github.com/ukaea/parallel-preprocessor/issues>,  will be the major channel for community interaction. However, user can directly email/slack the authors on topics not suitable to discuss in issue boards. 

If you want fix the bug, please nominate yourself when raise the bug. 

### Workflow for contribution

1. Fork on github `https://github.com/ukaea/parallel-preprocessor`
`git clone -b main https://github.com/your_gihtub_user_name/parallel-preprocessor`
setup upstream by `git remote set-url upstream https://github.com/ukaea/parallel-preprocessor` 

Note:  the main branch of this repo is `main` not `master`. 

2. Get the latest main branch source code
`cd parallel-preprocessor`
`git checkout main && git fetch upstream main`

3. Develop your feature or fix in a new branch
`git check -b your_feature_branch`

4. Rebase the upstream main before send in PR

This may involved manually merge. 
Some links and tools to help git merge will be added later here.

5. squash tiny fix after CI building test and code review
`git reset --cache HEAD~1`  uncommit the last commit
make the fix, then force push by  `git push origin +your_feature_branch`

This will save contributor to squash tiny fixes and make the commit history clean.

6. merged to main, Congratulaton! 


### Put in PR
By putting in PR, it is assumed contributor(s) **give the maintainer the legal rights to change the open source license in the future**. There is a PR template showing a checklist, which can be updated [pull_request_template](../.github/pull_request_template.md)

### License consideration

This parallel-preprocessor project is open source under the  LGPL v2.1 instead of LGPL v3, because all relevant projects, OCCT, salome, FreeCAD, are using LGPL v2.1. Meanwhile, LGPL v3 and LGPL v2.1 is not fully compatible to mixed up in source code level. In order to be co-operative with other relavent projects to build up an open source  CAD-CAE automated workflow, the same license LGPLv2.1 is adopted. 

In the future, this project may be released in a new open source licese, or even dual license, if relicense may benefit the project sustainability. 




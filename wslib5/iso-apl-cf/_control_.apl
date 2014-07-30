⍝ This is a package control file.

⍝ Decline to load this file unless the package manager is present.
⍎(0=⎕nc'pkg∆manager')/'''Load using the APL Package Manager.'' ◊ →'

⍝ The following information applies to the APL Package Manager release
⍝ named "Netochka".
⍝
⍝ A package consists of a directory which in turn contains files or
⍝ other directories. The files, with the exception of the control and
⍝ metadata files, may contain anything. The directories may be used to
⍝ organize other files and directories in any manner. The only requirements
⍝ of the packaging scheme are the top-level directory (nominally named for
⍝ the package) and the control and metadata files.
⍝
⍝ For purposes of transport and distribution, a package may be bundled
⍝ in a .zip file. The zip file should bear the name of the package
⍝ directory.
⍝
⍝ A package may be installed via other means, such as from a version
⍝ control repository such as used by svn or git. The presence of version
⍝ control data in the package directory shall not influence the function
⍝ of the package manager.
⍝ 
⍝ The package control file is just a Unicode representation of a valid
⍝ APL program. The control file may be complete in and of itself or
⍝ may run code to construct, load and install the package using additional
⍝ files and tools.
⍝
⍝ A package may depend on other packages, but must not access or
⍝ create files in other package directories.
⍝
⍝ It's perfectly fine for a package to read or install files in
⍝ locations other than in package directories. Of course, you must
⍝ qualify external locations based upon platform probes.
⍝
⍝ The following table lists the API functions that you may use to
⍝ facilitate loading of a package. Refer to API.md for detailed
⍝ descriptions.
⍝
⍝ | Function            | Use                                   | Phase |
⍝ | ------------------- | ------------------------------------- | ----- |
⍝ | pkg∆manager         | package manager release               | Q     |
⍝ | pkg∆platform_family | host platform kind                    | R     |
⍝ | pkg∆os_type         | host OS identification                | R     |
⍝ | pkg∆os_distribution | host OS identification                | R     |
⍝ | pkg∆os_version      | host OS identification                | R     |
⍝ | pkg∆apl_type        | APL system identification             | R     |
⍝ | pkg∆apl_version     | APL system identification             | R     |
⍝ | pkg∆shell_type      | host shell identification             | R     |
⍝ | pkg∆shell_version   | host shell identification             | R     |
⍝ | pkg∆case            | select alternatives functions         | L     |
⍝ | pkg∆compare_version | test relation between version numbers | L     |
⍝ | pkg∆shell           | run host shell command                | L     |
⍝ | pkg∆copy            | copy APL code into workspace          | L     |
⍝ | pkg∆metadata        | query current package's metadata      | L     |
⍝ | pkg∆file            | query host filesystem objects         | L     |
⍝
⍝ Phases
⍝ ------
⍝ Q: Query - Determine the presence of the package manager via
⍝   `0≠⎕nc 'pkg∆manager'`.
⍝ L: Load-time - These functions may only be called during package
⍝    loading.
⍝ R: Run-time - These functions may be called at package run time if
⍝    guarded by a query for package manager presence. These functions
⍝    may also be called during package loading.

pkg∆copy 'iso_cf.apl'

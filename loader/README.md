# ASI loader for packaging

The source repository does not vendor the loader binary. Building SA-People.asi does not
require it. To create a full installer with MakeRelease.ps1, place the 32-bit `dinput8.dll`
from [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases) here.
Keep Ultimate-ASI-Loader-LICENSE.txt with the distributed binary.

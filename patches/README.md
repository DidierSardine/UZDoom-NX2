# Engine patch series

`engine/` = UZDoom 5.0.3 (official tag `5.0.3`,
`7910df7102033fd7754a317f3254f24101135926`)
+ the 66 commits of RDoom NX (Switch 1 port, github.com/aeon1911/RDoom-NX-engine)
+ the UZDoom NX2 commits (Switch 2 port), at the end of the series.

Rebuild `engine/`:

```sh
git clone https://github.com/UZDoom/UZDoom engine
git -C engine checkout -b uzdoom-nx2 7910df7102033fd7754a317f3254f24101135926
git -C engine am ../patches/*.patch
```

`SHA256SUMS.txt` lists the checksums of the patch files.

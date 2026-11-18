# Série de patches du moteur

`engine/` = UZDoom 5.0.3 (tag `7910df7102033fd7754a317f3254f24101135926`)
+ 66 commits de RDoom NX (portage Switch 1, github.com/aeon1911/RDoom-NX-engine)
+ les commits UZDoom NX2 (portage Switch 2), en fin de série.

Reconstruire `engine/` :

```sh
git clone https://github.com/UZDoom/uzdoom engine
git -C engine checkout -b uzdoom-nx2 7910df7102033fd7754a317f3254f24101135926
git -C engine am ../patches/*.patch
```

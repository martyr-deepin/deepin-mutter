<a name=""></a>
##  3.20.38 (2018-12-26)

#### Features

* x11: Do check if time window is valid ([36e34318](36e34318))
* x11: Double check whether the user time window was previously used ([ee6b5797](ee6b5797))



<a name=""></a>
##  3.20.37 (2018-12-17)


#### Bug Fixes

* **workaround:**  prevent weird touch end behaviour ([cb910cb4](cb910cb4))



<a name="3.20.36"></a>
## 3.20.36 (2018-12-07)


#### Bug Fixes

*   ungrab after double click handler ([aa48d349](aa48d349))



<a name="3.20.35"></a>
## 3.20.35 (2018-11-09)


#### Bug Fixes

*   theme rendering blurry in scaled mode ([fdd731a7](fdd731a7), closes [#99](99))



<a name=""></a>
##  3.20.34 (2018-08-10)


#### Bug Fixes

*   disable pair resizing for dual-tiling mode ([7ec798d9](7ec798d9))



<a name=""></a>
##  3.20.33 (2018-08-10)


#### Bug Fixes

* **workaround:**  support drag-to-move by CM ([d010d004](d010d004))



<a name=""></a>
##  3.20.32 (2018-08-01)


#### Bug Fixes

*   better texture error handling ([bb7f60f4](bb7f60f4))



<a name=""></a>
##  3.20.31 (2018-07-20)


#### Bug Fixes

*   disable settings update on startup ([906c2310](906c2310))
* **workaround:**  disable monitor-aware unredirect ([5b6f0364](5b6f0364))


##  3.20.30 (2018-05-14)


#### Bug Fixes

*   fix cogl resources leak ([46e8d6de](46e8d6de))
*   deref texture causes crash when screen changes ([5e736ffc](5e736ffc))
*   update interactive tile check ([1df8b10d](1df8b10d))

#### Features

*   pair resizing for dual tiling windows ([518648fc](518648fc))
*   support tile effect ([4524e5a0](4524e5a0))
*   use flag to control background blurring ([ba57c864](ba57c864))
* **tile:**
  *  honor size limits when do tile resizing ([f40bdb6f](f40bdb6f))
  *  allow directional resize in tiling mode ([b5dd57b6](b5dd57b6))



##  3.20.29 (2018-03-22)


#### Bug Fixes

*   reduce texture recreation ([317b09df](317b09df))
*   keep wm_state of OR window when unmanaged ([f2a831b0](f2a831b0))

#### Features

*   expand available region for CSD window ([1cedcd7f](1cedcd7f))



##  3.20.28 (2018-03-16)


#### Features

*   fine-grained control if texture needs recreate ([dbfd9a5a](dbfd9a5a))



##  3.20.27 (2018-03-07)


#### Bug Fixes

*   cache result for is_run_on_livecd ([b831ee84](b831ee84))
*   Adapt lintian ([d59de6e5](d59de6e5))
*   use mask surface with owned data ([5b943d63](5b943d63))



##  3.20.26 (2017-11-16)


#### Bug Fixes

*   check validity of blur related coordinates ([bec56108](bec56108))

#### Features

*   support FLATPAK_APPID property ([7dc6e653](7dc6e653))



##  3.20.25 (2017-11-09)


#### Bug Fixes

*   remove keep_at_edge constraints ([18e5cbec](18e5cbec))



##  3.20.24 (2017-11-06)


#### Bug Fixes

*   set tile monitor ([acad3597](acad3597))



## 3.20.23 (2017-11-03)

#### Features

*   add two public operations for window ([a8fa43cc](a8fa43cc))

#### Bug Fixes

*   compile with gudev 232 ([f718cac3](f718cac3))
*   correct texture coordinate calculation ([45cb1972](45cb1972))
*   check if tile is permitted ([d87d6875](d87d6875))



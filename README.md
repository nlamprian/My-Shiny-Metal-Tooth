My-Shiny-Metal-Tooth
====================

<img src="https://github.com/nlamprian/My-Shiny-Metal-Tooth/wiki/assets/sign.jpg" border="0" align="right" height="270" hspace="10" alt="promo"/>

`My Shiny Metal Tooth` is an **autonomous illuminated sign** I built for my father’s dental clinic.

A **5 mm** thick **anodized wire** is used to build a **frame**. The **lighting** is realized with **EL Wire**. The whole sign operates autonomously. A **DS1307 RTC** module is used for timekeeping. The **sunset time** is calculated for each day, so the system knows when to turn on. An **Arduino Leonardo** controls the logic.

For more details on the design and construction of the sign, take a look at the tutorial on the [wiki](https://github.com/nlamprian/My-Shiny-Metal-Tooth/wiki/Tutorial).

Attribution
-----------

The project uses a slightly modified version of the [TimeLord](http://swfltek.com/arduino/timelord.html) library.

There is also a dependency on the [Jeelab RTClib](https://github.com/adafruit/RTClib) library.

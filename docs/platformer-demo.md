---
icon: material/gamepad-square
hide:
    toc: true
---

# Platformer Demo

This is a port of the [XNA Platformer Starter Kit](https://github.com/SimonDarksideJ/XNAGameStudio/tree/archive/Samples/Platformer_4_0), made with cerlib and compiled to WebAssembly using [Emscripten](https://emscripten.org/). (1)
{.annotate}

1. If you are interested in how to build your game for the Web, see [Deploying to WebAssembly](deployment/deploying-to-web.md)

<div class="dbcanvascontainer">
  <canvas id="canvas" class="dbcanvas" oncontextmenu="event.preventDefault()"></canvas>
</div>

<script type='text/javascript'>
  var Module = {
    canvas: (function () {
      return document.getElementById('canvas');
    })(),
    locateFile: filename => {
      return 'https://cerlib.org/platformerdemo/' + filename
    }
  };
</script>

<script src="https://cerlib.org/platformerdemo/Platformer.js"></script>

## Controls

| Action            | Key                               |
|-------------------|-----------------------------------|
| Move left / right | ++a++, ++d++, ++left++, ++right++ |
| Jump              | ++space++                         |

_(Sorry, this demo is not yet made for mobile devices :fontawesome-solid-face-sad-tear:)_
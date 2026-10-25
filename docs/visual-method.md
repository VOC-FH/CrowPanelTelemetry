# Visual production method

The built-in image generation tool was first asked to preserve the uploaded
photographs and overlay the verified J7/J2/J13 net mapping. Its draft moved wire
endpoints and was explicitly rejected. That draft is not in this project and must
not be used to build the circuit.

The accepted diagram is a deterministic vector overlay, produced by
`tools/draw_wiring.py`, from the unchanged `docs/source-photo.png`. The complete
geometric/net-label specification is the reproducible script; its source image
bytes are embedded unchanged into the standalone SVG. Pinning/trace verification
is separate in `tools/check_wiring.py` and `docs/wiring-verification.txt`.
GPS connections terminate at a labelled breakout because the photo does not
establish the exact GPS pad identities. The power link remains conditional.

The final wiring SVG was rasterized by Inkscape to wiring.png. The UI preview
uses `tools/draw_ui.py` with the firmware's dashboard geometry and illustrative
data, then Inkscape renders ui-preview.png. Neither accepted artifact uses an
AI-generated hardware photograph or an invented successful live measurement.

To regenerate from the project folder:

```
python3 tools/draw_wiring.py
python3 tools/draw_ui.py
inkscape docs/wiring.svg --export-type=png --export-filename=docs/wiring.png
inkscape docs/ui-preview.svg --export-type=png --export-filename=docs/ui-preview.png
```

Final assets: `docs/wiring.png`, `docs/wiring.svg`, `docs/ui-preview.png`,
`docs/ui-preview.svg`. The original photo and both render scripts are included.

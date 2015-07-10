; Export tools for Kobo Redux
; Copyright 2015 David Olofson

(define (resize-centered img width height)
	(gimp-image-resize
		img
		width height
		(/ (- width (car (gimp-image-width img))) 2)
		(/ (- height (car (gimp-image-height img))) 2))
)

(define (delete-hidden-layers img) 
	(let*
		(
			(lrs (gimp-image-get-layers img))
			(nlayers (car lrs))
			(layers (cadr lrs))
			(i 0)
			(layer 0)
		)
		(set! i 0)
		(while (< i nlayers)
			(set! layer (aref layers i))
			(if (= (car (gimp-item-get-visible layer)) FALSE)
				(gimp-image-remove-layer img layer)
			)
			(set! i (+ i 1))
		)
	)
)

(define (export-koboredux-texture img
		filename
		inSize outSize
		sunlight omnisun
		brightness contrast
		blur sharpen)
	(let*
		(
			(Image (car (gimp-image-duplicate img)))
			(dLayer 0)
			(sLayer 0)
			(osLayer 0)
			(Drawable 0)
			(filterScale (/ inSize outSize))
		)

		(gimp-image-undo-disable Image)

		;Set canvas size to the area of interest
		(resize-centered Image inSize inSize)

		;Apply sunlight level
		(set! sLayer (car (gimp-image-get-layer-by-name Image
			"Sunlight")))
		(when (>= sLayer 0)
			(gimp-layer-set-opacity sLayer sunlight)
			(gimp-item-set-visible sLayer TRUE)
		)

		;Apply omnisun level
		(set! osLayer (car (gimp-image-get-layer-by-name Image
			"Omnisun")))
		(when (>= osLayer 0)
			(gimp-layer-set-opacity osLayer omnisun)
			(gimp-item-set-visible osLayer TRUE)
		)

		;Delete any remaining hidden layers, to save CPU!
		(delete-hidden-layers Image)

		;Flatten without resizing anything
		(set! Drawable
			(car (gimp-image-merge-visible-layers
				Image EXPAND-AS-NECESSARY)))
		(gimp-layer-flatten Drawable)

		;Blur, if desired
		(when (> blur 0)
			(plug-in-gauss
				RUN-NONINTERACTIVE
				Image
				Drawable
				(* blur filterScale)
				(* blur filterScale)
				0)
		)

		;Sharpen, if desired
		(when (> sharpen 0)
			(plug-in-unsharp-mask
				RUN-NONINTERACTIVE
				Image
				Drawable
				0.5
				sharpen
				0)
		)

		;Scale
		(unless (equal? inSize outSize)
			(gimp-context-set-interpolation INTERPOLATION-CUBIC)
			(gimp-image-scale Image outSize outSize)
		)

		;Flatten
		(gimp-image-flatten Image)
		(set! Drawable
			(aref (cadr (gimp-image-get-layers Image)) 0))

		; Brightness/contrast
		(unless (and (equal? brightness 0) (equal? contrast 0))
			(gimp-drawable-brightness-contrast
				Drawable brightness contrast))

		;Convert to DO64-0.24
		(gimp-image-convert-indexed
			Image
			NO-DITHER
			CUSTOM-PALETTE
			0
			FALSE
			FALSE
			"DO64-0.24"
		)

		;Save!
		(file-png-save2
			RUN-NONINTERACTIVE
			Image
			Drawable
			filename
			filename
			FALSE		;interlace
			9
			FALSE		;bKGD
			FALSE		;gAMA
			FALSE		;oFFs
			FALSE		;pHYs
			FALSE		;tIME
			FALSE		;comment
			FALSE		;preserve transparent
		)
	)
)

(define (export-koboredux-ground-r1l10 img)
	(export-koboredux-texture img
		"ground-r1l10.png"
		1024	768
		0	50
		-0.1	0
		0.0	0.7)
)
(define (export-koboredux-ground-r1l9 img)
	(export-koboredux-texture img
		"ground-r1l9.png"
		1024	512
		0	52
		-0.15	0
		0.8	0.0)
)
(define (export-koboredux-ground-r1l8 img)
	(export-koboredux-texture img
		"ground-r1l8.png"
		1024	384
		0	54
		-0.2	0
		1.5	0.0)
)

(define (export-koboredux-planet-r1l6 img)
	(export-koboredux-texture img
		"planet-r1l6.png"
		2048	512
		56	0
		0	0
		2.1	0)
)
(define (export-koboredux-planet-r1l4 img)
	(export-koboredux-texture img
		"planet-r1l4.png"
		2048	256
		59	0
		0.1	0.1
		2.5	0)
)
(define (export-koboredux-planet-r1l3 img)
	(export-koboredux-texture img
		"planet-r1l3.png"
		2048	128
		62	0
		0.2	0.2
		2.6	0)
)
(define (export-koboredux-planet-r1l1 img)
	(export-koboredux-texture img
		"planet-r1l1.png"
		2048	64
		65	0
		0.3	0.3
		2.7	0)
)

(define (export-koboredux-ground-r1 img)
	(export-koboredux-ground-r1l10 img)
	(export-koboredux-ground-r1l9 img)
	(export-koboredux-ground-r1l8 img)
)

(define (export-koboredux-planet-r1 img)
	(export-koboredux-planet-r1l6 img)
	(export-koboredux-planet-r1l4 img)
	(export-koboredux-planet-r1l3 img)
	(export-koboredux-planet-r1l1 img)
)

(define (export-koboredux-all-r1 img)
	(export-koboredux-ground-r1 img)
	(export-koboredux-planet-r1 img)
)

(script-fu-register
	"export-koboredux-ground-r1"
	"Export Backgrounds"
	"Exports scrolling backgrounds for Levels 8-10."
	"David Olofson"
	"Copyright 2015, David Olofson"
	"2015-07-08"
	"INDEXED* RGB* GRAY*"
	SF-IMAGE	"Image to export"	0
)
(script-fu-menu-register
	"export-koboredux-ground-r1"
	"<Image>/File/Export/Kobo Redux/Region 1"
)

(script-fu-register
	"export-koboredux-planet-r1"
	"Export Planet Textures"
	"Exports rotating planet textures for Levels 1-7."
	"David Olofson"
	"Copyright 2015, David Olofson"
	"2015-07-08"
	"INDEXED* RGB* GRAY*"
	SF-IMAGE	"Image to export"	0
)
(script-fu-menu-register
	"export-koboredux-planet-r1"
	"<Image>/File/Export/Kobo Redux/Region 1"
)

(script-fu-register
	"export-koboredux-all-r1"
	"Export All"
	"Exports all assets for Levels 1-10."
	"David Olofson"
	"Copyright 2015, David Olofson"
	"2015-07-08"
	"INDEXED* RGB* GRAY*"
	SF-IMAGE	"Image to export"	0
)
(script-fu-menu-register
	"export-koboredux-all-r1"
	"<Image>/File/Export/Kobo Redux/Region 1"
)

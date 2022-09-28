;GIL-3
(
(
	:type                   :menu
	:name                   acc_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Accuracy"
	:menu-item-labels       ("0 radix places" "1 radix places" "2 radix places" "3 radix places" "4 radix places" "5 radix places" "6 radix places" "7 radix places" "8 radix places" "9 radix places" )
	:menu-item-label-types  (:string :string :string :string :string :string :string :string :string :string )
	:menu-item-states       (:active :active :active :active :active :active :active :active :active :active )
	:menu-item-defaults     (nil nil nil nil nil nil nil nil nil nil )
	:initial-selections     (nil nil nil nil nil nil nil nil nil nil )
	:menu-item-handlers     (nil nil nil nil nil nil nil nil nil nil )
	:menu-item-menus        (nil nil nil nil nil nil nil nil nil nil )
	:menu-item-colors       ("" "" "" "" "" "" "" "" "" "" )
	:pinnable               t
	:user-data              ()
	:actions                ()
)
(
	:type                   :menu
	:name                   exch_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Exchange"
	:menu-item-labels       ("Register 0" "Register 1" "Register 2" "Register 3" "Register 4" "Register 5" "Register 6" "Register 7" "Register 8" "Register 9" )
	:menu-item-label-types  (:string :string :string :string :string :string :string :string :string :string )
	:menu-item-states       (:active :active :active :active :active :active :active :active :active :active )
	:menu-item-defaults     (nil nil nil nil nil nil nil nil nil nil )
	:initial-selections     (nil nil nil nil nil nil nil nil nil nil )
	:menu-item-handlers     (nil nil nil nil nil nil nil nil nil nil )
	:menu-item-menus        (nil nil nil nil nil nil nil nil nil nil )
	:menu-item-colors       ("" "" "" "" "" "" "" "" "" "" )
	:pinnable               t
	:user-data              ()
	:actions                ()
)
(
	:type                   :menu
	:name                   lshift_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Left shift"
	:menu-item-labels       ("1 place" "2 places" "3 places" "4 places" "5 places" "6 places" "7 places" "8 places" "9 places" "10 places" "11 places" "12 places" "13 places" "14 places" "15 places" )
	:menu-item-label-types  (:string :string :string :string :string :string :string :string :string :string :string :string :string :string :string )
	:menu-item-states       (:active :active :active :active :active :active :active :active :active :active :active :active :active :active :active )
	:menu-item-defaults     (nil nil nil nil nil nil nil nil nil nil nil nil nil nil nil )
	:initial-selections     (nil nil nil nil nil nil nil nil nil nil nil nil nil nil nil )
	:menu-item-handlers     (nil nil nil nil nil nil nil nil nil nil nil nil nil nil nil )
	:menu-item-menus        (nil nil nil nil nil nil nil nil nil nil nil nil nil nil nil )
	:menu-item-colors       ("" "" "" "" "" "" "" "" "" "" "" "" "" "" "" )
	:pinnable               t
	:user-data              ()
	:actions                ()
)
(
	:type                   :menu
	:name                   base_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Numeric base"
	:menu-item-labels       ("Binary" "Octal" "Decimal" "Hexadecimal" )
	:menu-item-label-types  (:string :string :string :string )
	:menu-item-states       (:active :active :active :active )
	:menu-item-defaults     (nil nil nil nil )
	:initial-selections     (nil nil nil nil )
	:menu-item-handlers     (nil nil nil nil )
	:menu-item-menus        (nil nil nil nil )
	:menu-item-colors       ("" "" "" "" )
	:pinnable               t
	:user-data              ()
	:actions                ()
)
(
	:type                   :menu
	:name                   disp_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Display type"
	:menu-item-labels       ("Engineering" "Fixed point" "Scientific" )
	:menu-item-label-types  (:string :string :string )
	:menu-item-states       (:active :active :active )
	:menu-item-defaults     (nil nil nil )
	:initial-selections     (nil nil nil )
	:menu-item-handlers     (nil nil nil )
	:menu-item-menus        (nil nil nil )
	:menu-item-colors       ("" "" "" )
	:pinnable               t
	:user-data              ()
	:actions                ()
)
(
	:type                   :menu
	:name                   trig_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Trigonometric type"
	:menu-item-labels       ("Degrees" "Gradients" "Radians" )
	:menu-item-label-types  (:string :string :string )
	:menu-item-states       (:active :active :active )
	:menu-item-defaults     (nil nil nil )
	:initial-selections     (nil nil nil )
	:menu-item-handlers     (nil nil nil )
	:menu-item-menus        (nil nil nil )
	:menu-item-colors       ("" "" "" )
	:pinnable               t
	:user-data              ()
	:actions                ()
)
(
	:type                   :menu
	:name                   mode_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Mode"
	:menu-item-labels       ("Basic" "Financial" "Logical" "Scientific" )
	:menu-item-label-types  (:string :string :string :string )
	:menu-item-states       (:active :active :active :active )
	:menu-item-defaults     (nil nil nil nil )
	:initial-selections     (nil nil nil nil )
	:menu-item-handlers     (nil nil nil nil )
	:menu-item-menus        (nil nil nil nil )
	:menu-item-colors       ("" "" "" "" )
	:pinnable               t
	:user-data              ()
	:actions                ()
)
(
	:type                   :menu
	:name                   props_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Calculator"
	:menu-item-labels       ("Properties..." )
	:menu-item-label-types  (:string )
	:menu-item-states       (:active )
	:menu-item-defaults     (nil )
	:initial-selections     (nil )
	:menu-item-handlers     (nil )
	:menu-item-menus        (nil )
	:menu-item-colors       ("" )
	:pinnable               nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :menu
	:name                   rcl_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Retrieve"
	:menu-item-labels       ("Register 0" "Register 1" "Register 2" "Register 3" "Register 4" "Register 5" "Register 6" "Register 7" "Register 8" "Register 9" )
	:menu-item-label-types  (:string :string :string :string :string :string :string :string :string :string )
	:menu-item-states       (:active :active :active :active :active :active :active :active :active :active )
	:menu-item-defaults     (nil nil nil nil nil nil nil nil nil nil )
	:initial-selections     (nil nil nil nil nil nil nil nil nil nil )
	:menu-item-handlers     (nil nil nil nil nil nil nil nil nil nil )
	:menu-item-menus        (nil nil nil nil nil nil nil nil nil nil )
	:menu-item-colors       ("" "" "" "" "" "" "" "" "" "" )
	:pinnable               t
	:user-data              ()
	:actions                ()
)
(
	:type                   :menu
	:name                   sto_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Store"
	:menu-item-labels       ("Register 0" "Register 1" "Register 2" "Register 3" "Register 4" "Register 5" "Register 6" "Register 7" "Register 8" "Register 9" )
	:menu-item-label-types  (:string :string :string :string :string :string :string :string :string :string )
	:menu-item-states       (:active :active :active :active :active :active :active :active :active :active )
	:menu-item-defaults     (nil nil nil nil nil nil nil nil nil nil )
	:initial-selections     (nil nil nil nil nil nil nil nil nil nil )
	:menu-item-handlers     (nil nil nil nil nil nil nil nil nil nil )
	:menu-item-menus        (nil nil nil nil nil nil nil nil nil nil )
	:menu-item-colors       ("" "" "" "" "" "" "" "" "" "" )
	:pinnable               t
	:user-data              ()
	:actions                ()
)
(
	:type                   :menu
	:name                   rshift_menu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Right shift"
	:menu-item-labels       ("1 place" "2 places" "3 places" "4 places" "5 places" "6 places" "7 places" "8 places" "9 places" "10 places" "11 places" "12 places" "13 places" "14 places" "15 places" )
	:menu-item-label-types  (:string :string :string :string :string :string :string :string :string :string :string :string :string :string :string )
	:menu-item-states       (:active :active :active :active :active :active :active :active :active :active :active :active :active :active :active )
	:menu-item-defaults     (nil nil nil nil nil nil nil nil nil nil nil nil nil nil nil )
	:initial-selections     (nil nil nil nil nil nil nil nil nil nil nil nil nil nil nil )
	:menu-item-handlers     (nil nil nil nil nil nil nil nil nil nil nil nil nil nil nil )
	:menu-item-menus        (nil nil nil nil nil nil nil nil nil nil nil nil nil nil nil )
	:menu-item-colors       ("" "" "" "" "" "" "" "" "" "" "" "" "" "" "" )
	:pinnable               t
	:user-data              ()
	:actions                ()
)
(
	:type                   :base-window
	:name                   kframe
	:owner                  nil
	:width                  408
	:height                 235
	:background-color       ""
	:foreground-color       ""
	:label                  "Calctool"
	:label-type             :string
	:initial-state          :open
	:show-footer            nil
	:resizable              t
	:icon-file              "images/calctool.icon"
	:icon-label             "calculator"
	:icon-mask-file         "images/calctool.mask.icon"
	:event-handler          frame_interpose
	:user-data              ()
	:actions                (
		(
		:from                   (kframe)
		:when                   (AnyEvent )
		:to                     (kframe)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (frame_interpose)
		)
	)
)
(
	:type                   :canvas-pane
	:name                   kcanvas
	:owner                  kframe
	:help                   ""
	:x                      0
	:y                      0
	:width                  408
	:height                 235
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:draggable              nil
	:droppable              nil
	:default-drop-site      nil
	:menu                   nil
	:horizontal-scrollbar   nil
	:scrollable-width       408
	:vertical-scrollbar     nil
	:scrollable-height      235
	:repaint-proc           canvas_repaint
	:event-handler          canvas_proc
	:drawing-model          :xwindows
	:user-data              ()
	:actions                (
		(
		:from                   (kframe kcanvas)
		:when                   (Repaint )
		:to                     (kframe kcanvas)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (canvas_repaint)
		)
		(
		:from                   (kframe kcanvas)
		:when                   (AnyEvent )
		:to                     (kframe kcanvas)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (canvas_proc)
		)
		(
		:from                   (kframe kcanvas)
		:when                   (Resize )
		:to                     (kframe kcanvas)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (canvas_resize)
		)
	)
)
(
	:type                   :popup-window
	:name                   mframe
	:owner                  kframe
	:width                  403
	:height                 66
	:background-color       ""
	:foreground-color       ""
	:label                  ""
	:label-type             :string
	:initial-state          :invisible
	:show-footer            nil
	:resizable              nil
	:pinned                 t
	:done-handler           nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :canvas-pane
	:name                   mcanvas
	:owner                  mframe
	:help                   ""
	:x                      0
	:y                      0
	:width                  403
	:height                 66
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:draggable              nil
	:droppable              nil
	:default-drop-site      nil
	:menu                   nil
	:horizontal-scrollbar   nil
	:scrollable-width       403
	:vertical-scrollbar     nil
	:scrollable-height      66
	:repaint-proc           canvas_repaint
	:event-handler          canvas_proc
	:drawing-model          :xwindows
	:user-data              ()
	:actions                (
		(
		:from                   (mframe mcanvas)
		:when                   (Repaint )
		:to                     (mframe mcanvas)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (canvas_repaint)
		)
		(
		:from                   (mframe mcanvas)
		:when                   (AnyEvent )
		:to                     (mframe mcanvas)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (canvas_proc)
		)
		(
		:from                   (mframe mcanvas)
		:when                   (Resize )
		:to                     (mframe mcanvas)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (canvas_resize)
		)
	)
)
(
	:type                   :popup-window
	:name                   Aframe
	:owner                  kframe
	:width                  156
	:height                 85
	:background-color       ""
	:foreground-color       ""
	:label                  "Get ASCII"
	:label-type             :string
	:initial-state          :invisible
	:show-footer            nil
	:resizable              nil
	:pinned                 t
	:done-handler           nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   Apanel
	:owner                  Aframe
	:help                   ""
	:x                      0
	:y                      0
	:width                  156
	:height                 85
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   Api_text
	:owner                  Apanel
	:help                   ""
	:x                      8
	:y                      8
	:width                  98
	:height                 13
	:value-x                74
	:value-y                8
	:value-length           1
	:stored-length          1
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Character:"
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       t
	:initial-value          ""
	:initial-state          :active
	:read-only              nil
	:notify-handler         tshow_ascii
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (Aframe Api_text)
		:when                   (Notify )
		:to                     (Aframe Api_text)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (tshow_ascii)
		)
	)
)
(
	:type                   :message
	:name                   Adummy
	:owner                  Apanel
	:help                   ""
	:x                      25
	:y                      36
	:width                  41
	:height                 11
	:foreground-color       ""
	:label                  "dummy"
	:label-type             :string
	:label-bold             t
	:initial-state          :invisible
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   Api_but
	:owner                  Apanel
	:help                   ""
	:x                      24
	:y                      62
	:width                  42
	:height                 18
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "ASCII"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         bshow_ascii
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (Aframe Api_but)
		:when                   (Notify )
		:to                     (Aframe Api_but)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (bshow_ascii)
		)
	)
)
(
	:type                   :group
	:name                   Agroup
	:x                      8
	:y                      8
	:group-type             :column
	:members                (Api_text Adummy Api_but )
	:rows                   0
	:row-alignment          :top-edges
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       15
	:anchor-object          nil
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      5
	:vertical-offset        5
	:user-data              ()
	:actions                ()
)
(
	:type                   :popup-window
	:name                   rframe
	:owner                  kframe
	:width                  248
	:height                 179
	:background-color       ""
	:foreground-color       ""
	:label                  "Memory Registers"
	:label-type             :string
	:initial-state          :invisible
	:show-footer            nil
	:resizable              nil
	:pinned                 t
	:done-handler           nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :canvas-pane
	:name                   rcanvas
	:owner                  rframe
	:help                   ""
	:x                      0
	:y                      0
	:width                  248
	:height                 179
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:draggable              nil
	:droppable              nil
	:default-drop-site      nil
	:menu                   nil
	:horizontal-scrollbar   nil
	:scrollable-width       248
	:vertical-scrollbar     nil
	:scrollable-height      179
	:repaint-proc           canvas_repaint
	:event-handler          canvas_proc
	:drawing-model          :xwindows
	:user-data              ()
	:actions                (
		(
		:from                   (rframe rcanvas)
		:when                   (Repaint )
		:to                     (rframe rcanvas)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (canvas_repaint)
		)
		(
		:from                   (rframe rcanvas)
		:when                   (AnyEvent )
		:to                     (rframe rcanvas)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (canvas_proc)
		)
		(
		:from                   (rframe rcanvas)
		:when                   (Resize )
		:to                     (rframe rcanvas)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (canvas_resize)
		)
	)
)
(
	:type                   :popup-window
	:name                   CFframe
	:owner                  kframe
	:width                  254
	:height                 141
	:background-color       ""
	:foreground-color       ""
	:label                  "New Constant"
	:label-type             :string
	:initial-state          :invisible
	:show-footer            nil
	:resizable              nil
	:pinned                 t
	:done-handler           nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   CFpanel
	:owner                  CFframe
	:help                   ""
	:x                      0
	:y                      0
	:width                  254
	:height                 141
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   CFpi_cftext
	:owner                  CFpanel
	:help                   ""
	:x                      31
	:y                      8
	:width                  111
	:height                 13
	:value-x                110
	:value-y                8
	:value-length           2
	:stored-length          2
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Constant no:"
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       t
	:initial-value          ""
	:initial-state          :active
	:read-only              nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   CFpi_dtext
	:owner                  CFpanel
	:help                   ""
	:x                      33
	:y                      41
	:width                  209
	:height                 13
	:value-x                110
	:value-y                41
	:value-length           22
	:stored-length          256
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Description:"
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       t
	:initial-value          ""
	:initial-state          :active
	:read-only              nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   CFpi_vtext
	:owner                  CFpanel
	:help                   ""
	:x                      67
	:y                      74
	:width                  175
	:height                 13
	:value-x                110
	:value-y                74
	:value-length           22
	:stored-length          256
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Value:"
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       t
	:initial-value          ""
	:initial-state          :active
	:read-only              nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   CFpi_cbut
	:owner                  CFpanel
	:help                   ""
	:x                      12
	:y                      107
	:width                  90
	:height                 18
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Enter Constant"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         write_cf_value
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (CFframe CFpi_cbut)
		:when                   (Notify )
		:to                     (CFframe CFpi_cbut)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (write_cf_value)
		)
	)
)
(
	:type                   :group
	:name                   CFgroup
	:x                      12
	:y                      8
	:group-type             :column
	:members                (CFpi_cftext CFpi_dtext CFpi_vtext CFpi_cbut )
	:rows                   0
	:row-alignment          :top-edges
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       20
	:anchor-object          nil
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      5
	:vertical-offset        5
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   CFpi_fbut
	:owner                  CFpanel
	:help                   ""
	:x                      148
	:y                      112
	:width                  88
	:height                 18
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Enter Function"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         write_cf_value
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (CFframe CFpi_fbut)
		:when                   (Notify )
		:to                     (CFframe CFpi_fbut)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (write_cf_value)
		)
	)
)
(
	:type                   :popup-window
	:name                   Pframe
	:owner                  kframe
	:width                  241
	:height                 171
	:background-color       ""
	:foreground-color       ""
	:label                  "Calculator properties"
	:label-type             :string
	:initial-state          :invisible
	:show-footer            nil
	:resizable              nil
	:pinned                 t
	:done-handler           nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   Ppanel
	:owner                  Pframe
	:help                   ""
	:x                      0
	:y                      0
	:width                  241
	:height                 171
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   Pappearance
	:owner                  Ppanel
	:help                   ""
	:x                      8
	:y                      12
	:width                  182
	:height                 21
	:value-x                85
	:value-y                12
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :exclusive
	:selection-required     t
	:label                  "Appearance:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("2D-look" "3D-look" )
	:choice-label-types     (:string :string )
	:choice-colors          ("" "" )
	:initial-selections     (nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   Pdisplay
	:owner                  Ppanel
	:help                   ""
	:x                      32
	:y                      48
	:width                  167
	:height                 21
	:value-x                85
	:value-y                48
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :exclusive
	:selection-required     t
	:label                  "Display:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("color" "monochrome" )
	:choice-label-types     (:string :string )
	:choice-colors          ("" "" )
	:initial-selections     (nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   Pstyle
	:owner                  Ppanel
	:help                   ""
	:x                      44
	:y                      84
	:width                  189
	:height                 21
	:value-x                85
	:value-y                84
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :exclusive
	:selection-required     t
	:label                  "Style:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("left-handed" "right-handed" )
	:choice-label-types     (:string :string )
	:choice-colors          ("" "" )
	:initial-selections     (nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   Pgroup2
	:x                      8
	:y                      12
	:group-type             :column
	:members                (Pappearance Pdisplay Pstyle )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       15
	:anchor-object          nil
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      0
	:vertical-offset        0
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   Papply
	:owner                  Ppanel
	:help                   ""
	:x                      17
	:y                      140
	:width                  44
	:height                 18
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Apply"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         prop_apply
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (Pframe Papply)
		:when                   (Notify )
		:to                     (Pframe Papply)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (prop_apply)
		)
	)
)
(
	:type                   :button
	:name                   Pdefaults
	:owner                  Ppanel
	:help                   ""
	:x                      71
	:y                      140
	:width                  99
	:height                 18
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Save as Defaults"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         prop_defaults
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (Pframe Pdefaults)
		:when                   (Notify )
		:to                     (Pframe Pdefaults)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (prop_defaults)
		)
	)
)
(
	:type                   :button
	:name                   Preset
	:owner                  Ppanel
	:help                   ""
	:x                      180
	:y                      140
	:width                  43
	:height                 18
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Reset"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         prop_reset
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (Pframe Preset)
		:when                   (Notify )
		:to                     (Pframe Preset)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (prop_reset)
		)
	)
)
(
	:type                   :group
	:name                   Pgroup1
	:x                      17
	:y                      140
	:group-type             :row
	:members                (Papply Pdefaults Preset )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          nil
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      0
	:vertical-offset        0
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   Pgroup3
	:x                      8
	:y                      12
	:group-type             :column
	:members                (Pgroup2 Pgroup1 )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :vertical-centers
	:horizontal-spacing     10
	:vertical-spacing       35
	:anchor-object          nil
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      0
	:vertical-offset        0
	:user-data              ()
	:actions                ()
)
)

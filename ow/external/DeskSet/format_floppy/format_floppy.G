;GIL-3
(
(
	:type                   :base-window
	:name                   base_frame
	:owner                  nil
	:width                  400
	:height                 150
	:background-color       ""
	:foreground-color       ""
	:label                  "Format Program"
	:label-type             :string
	:initial-state          :open
	:show-footer            t
	:resizable              t
	:icon-file              ""
	:icon-label             ""
	:icon-mask-file         ""
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :popup-window
	:name                   format_popup
	:owner                  base_frame
	:width                  534
	:height                 343
	:background-color       ""
	:foreground-color       ""
	:label                  "Format Floppy"
	:label-type             :string
	:initial-state          :invisible
	:show-footer            t
	:resizable              t
	:pinned                 t
	:done-handler           nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   format_panel
	:owner                  format_popup
	:help                   "\"Format Floppy\" popup panel area."
	:x                      0
	:y                      0
	:width                  534
	:height                 343
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
	:type                   :message
	:name                   format_msg_1
	:owner                  format_panel
	:help                   ""
	:x                      130
	:y                      50
	:width                  340
	:height                 13
	:foreground-color       ""
	:label                  "Are you sure you want to format this floppy disk?"
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   format_msg_2
	:owner                  format_panel
	:help                   ""
	:x                      50
	:y                      80
	:width                  433
	:height                 13
	:foreground-color       ""
	:label                  "WARNING:   Formatting will destroy any information on the disk"
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   format_msg_3
	:owner                  format_panel
	:help                   ""
	:x                      130
	:y                      100
	:width                  139
	:height                 13
	:foreground-color       ""
	:label                  "and is not undoable."
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   format_choice
	:owner                  format_panel
	:help                   "Choice of three floppy formats."
	:x                      130
	:y                      142
	:width                  342
	:height                 69
	:value-x                293
	:value-y                142
	:rows                   3
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :exclusive
	:selection-required     t
	:label                  "Please select a format:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("UNIX" "DOS High Density" "NEC-DOS Medium Density" )
	:choice-label-types     (:string :string :string )
	:choice-colors          ("" "" "" )
	:initial-selections     (nil nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   format_diskname
	:owner                  format_panel
	:help                   "New disk label."
	:x                      130
	:y                      250
	:width                  325
	:height                 15
	:value-x                215
	:value-y                250
	:value-length           30
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Disk Name:"
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       t
	:initial-value          ""
	:initial-state          :active
	:read-only              nil
	:notify-handler         store_label
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (format_popup format_diskname)
		:when                   (Notify )
		:to                     (format_popup format_diskname)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (store_label)
		)
	)
)
(
	:type                   :button
	:name                   format_format
	:owner                  format_panel
	:help                   "Format the floppy disk."
	:x                      130
	:y                      280
	:width                  91
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Format Disk"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         format
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (format_popup format_format)
		:when                   (Notify )
		:to                     (format_popup format_format)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (format)
		)
	)
)
(
	:type                   :group
	:name                   group4
	:x                      50
	:y                      50
	:group-type             nil
	:members                (format_msg_1 format_msg_2 format_msg_3 format_choice format_diskname format_format )
	:rows                   0
	:row-alignment          :top-edges
	:columns                0
	:column-alignment       :left-edges
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
	:type                   :button
	:name                   format_cancel
	:owner                  format_panel
	:help                   "Format floppy popup goes away."
	:x                      360
	:y                      280
	:width                  59
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Cancel"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         cancel
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (format_popup format_cancel)
		:when                   (Notify )
		:to                     (format_popup format_cancel)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (cancel)
		)
	)
)
(
	:type                   :popup-window
	:name                   unlabeled_popup
	:owner                  base_frame
	:width                  546
	:height                 363
	:background-color       ""
	:foreground-color       ""
	:label                  "Unlabeled Floppy"
	:label-type             :string
	:initial-state          :invisible
	:show-footer            t
	:resizable              t
	:pinned                 t
	:done-handler           nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   unlabeled_panel
	:owner                  unlabeled_popup
	:help                   "\"Unlabeled Floppy\" popup panel area."
	:x                      0
	:y                      0
	:width                  546
	:height                 363
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
	:type                   :message
	:name                   unlabeled_msg_1
	:owner                  unlabeled_panel
	:help                   ""
	:x                      140
	:y                      40
	:width                  316
	:height                 13
	:foreground-color       ""
	:label                  "This floppy disk is unreadable by the system."
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   unlabeled_msg_2
	:owner                  unlabeled_panel
	:help                   ""
	:x                      140
	:y                      60
	:width                  273
	:height                 13
	:foreground-color       ""
	:label                  "It may contain tar or bar formatted data."
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   unlabeled_msg_3
	:owner                  unlabeled_panel
	:help                   ""
	:x                      140
	:y                      100
	:width                  172
	:height                 13
	:foreground-color       ""
	:label                  "Do you want to format it?"
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   unlabeled_msg_4
	:owner                  unlabeled_panel
	:help                   ""
	:x                      60
	:y                      140
	:width                  433
	:height                 13
	:foreground-color       ""
	:label                  "WARNING:   Formatting will destroy any information on the disk"
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   unlabeled_msg_5
	:owner                  unlabeled_panel
	:help                   ""
	:x                      140
	:y                      160
	:width                  139
	:height                 13
	:foreground-color       ""
	:label                  "and is not undoable."
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   format_choice
	:owner                  unlabeled_panel
	:help                   "Choice of three floppy formats."
	:x                      140
	:y                      200
	:width                  342
	:height                 69
	:value-x                303
	:value-y                200
	:rows                   3
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :exclusive
	:selection-required     t
	:label                  "Please select a format:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("UNIX" "DOS High Density" "NEC-DOS Medium Density" )
	:choice-label-types     (:string :string :string )
	:choice-colors          ("" "" "" )
	:initial-selections     (nil nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   format_diskname
	:owner                  unlabeled_panel
	:help                   "New disk label."
	:x                      140
	:y                      300
	:width                  325
	:height                 15
	:value-x                225
	:value-y                300
	:value-length           30
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Disk Name:"
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       t
	:initial-value          ""
	:initial-state          :active
	:read-only              nil
	:notify-handler         store_label
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (unlabeled_popup format_diskname)
		:when                   (Notify )
		:to                     (unlabeled_popup format_diskname)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (store_label)
		)
	)
)
(
	:type                   :button
	:name                   format_format
	:owner                  unlabeled_panel
	:help                   "Format the floppy disk."
	:x                      80
	:y                      340
	:width                  91
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Format Disk"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         unlabeled_format
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (unlabeled_popup format_format)
		:when                   (Notify )
		:to                     (unlabeled_popup format_format)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (unlabeled_format)
		)
	)
)
(
	:type                   :group
	:name                   group2
	:x                      60
	:y                      40
	:group-type             nil
	:members                (unlabeled_msg_1 unlabeled_msg_2 unlabeled_msg_3 unlabeled_msg_4 unlabeled_msg_5 format_choice format_diskname format_format )
	:rows                   1
	:row-alignment          :top-edges
	:columns                0
	:column-alignment       :left-edges
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
	:type                   :button
	:name                   format_cancel
	:owner                  unlabeled_panel
	:help                   "Format Floppy popup goes away."
	:x                      240
	:y                      340
	:width                  59
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Cancel"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         cancel
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (unlabeled_popup format_cancel)
		:when                   (Notify )
		:to                     (unlabeled_popup format_cancel)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (cancel)
		)
	)
)
(
	:type                   :button
	:name                   format_eject
	:owner                  unlabeled_panel
	:help                   "The popup goes away and the floppy is ejected."
	:x                      410
	:y                      340
	:width                  105
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Cancel & Eject"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         eject
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (unlabeled_popup format_eject)
		:when                   (Notify )
		:to                     (unlabeled_popup format_eject)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (eject)
		)
	)
)
(
	:type                   :popup-window
	:name                   unformatted_popup
	:owner                  base_frame
	:width                  546
	:height                 402
	:background-color       ""
	:foreground-color       ""
	:label                  "Unformatted Floppy"
	:label-type             :string
	:initial-state          :invisible
	:show-footer            t
	:resizable              t
	:pinned                 t
	:done-handler           nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   unformatted_panel
	:owner                  unformatted_popup
	:help                   "\"Unformatted Floppy\" popup panel area."
	:x                      0
	:y                      0
	:width                  546
	:height                 402
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
	:type                   :message
	:name                   unformatted_msg_1
	:owner                  unformatted_panel
	:help                   ""
	:x                      140
	:y                      40
	:width                  330
	:height                 13
	:foreground-color       ""
	:label                  "This floppy disk is unreadable.  It may be blank"
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   unformatted_msg_2
	:owner                  unformatted_panel
	:help                   ""
	:x                      140
	:y                      61
	:width                  276
	:height                 13
	:foreground-color       ""
	:label                  "or contain data in an unreadable format."
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   unformatted_group_2
	:x                      140
	:y                      40
	:group-type             :column
	:members                (unformatted_msg_1 unformatted_msg_2 )
	:rows                   0
	:row-alignment          :top-edges
	:columns                1
	:column-alignment       :left-edges
	:horizontal-spacing     10
	:vertical-spacing       5
	:anchor-object          nil
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      0
	:vertical-offset        0
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   unformatted_msg_3
	:owner                  unformatted_panel
	:help                   ""
	:x                      140
	:y                      90
	:width                  172
	:height                 13
	:foreground-color       ""
	:label                  "Do you want to format it?"
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   unformatted_msg_4
	:owner                  unformatted_panel
	:help                   ""
	:x                      60
	:y                      120
	:width                  433
	:height                 13
	:foreground-color       ""
	:label                  "WARNING:   Formatting will destroy any information on the disk"
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   unformatted_msg_5
	:owner                  unformatted_panel
	:help                   ""
	:x                      140
	:y                      140
	:width                  139
	:height                 13
	:foreground-color       ""
	:label                  "and is not undoable."
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   format_choice
	:owner                  unformatted_panel
	:help                   "Choice of three floppy formats."
	:x                      140
	:y                      190
	:width                  342
	:height                 69
	:value-x                303
	:value-y                190
	:rows                   3
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :exclusive
	:selection-required     t
	:label                  "Please select a format:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("UNIX" "DOS High Density" "NEC-DOS Medium Density" )
	:choice-label-types     (:string :string :string )
	:choice-colors          ("" "" "" )
	:initial-selections     (nil nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   format_diskname
	:owner                  unformatted_panel
	:help                   "New disk label."
	:x                      140
	:y                      290
	:width                  325
	:height                 15
	:value-x                225
	:value-y                290
	:value-length           30
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Disk Name:"
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       t
	:initial-value          ""
	:initial-state          :active
	:read-only              nil
	:notify-handler         store_label
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (unformatted_popup format_diskname)
		:when                   (Notify )
		:to                     (unformatted_popup format_diskname)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (store_label)
		)
	)
)
(
	:type                   :button
	:name                   format_format
	:owner                  unformatted_panel
	:help                   "Format the floppy disk."
	:x                      140
	:y                      330
	:width                  91
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Format Disk"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         unformatted_format
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (unformatted_popup format_format)
		:when                   (Notify )
		:to                     (unformatted_popup format_format)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (unformatted_format)
		)
	)
)
(
	:type                   :group
	:name                   group3
	:x                      60
	:y                      40
	:group-type             nil
	:members                (unformatted_group_2 unformatted_msg_3 unformatted_msg_4 unformatted_msg_5 format_choice format_diskname format_format )
	:rows                   0
	:row-alignment          :top-edges
	:columns                0
	:column-alignment       :left-edges
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
	:type                   :button
	:name                   format_cancel
	:owner                  unformatted_panel
	:help                   "Format Floppy popup goes away."
	:x                      260
	:y                      330
	:width                  59
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Cancel"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         cancel
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (unformatted_popup format_cancel)
		:when                   (Notify )
		:to                     (unformatted_popup format_cancel)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (cancel)
		)
	)
)
(
	:type                   :button
	:name                   format_eject
	:owner                  unformatted_panel
	:help                   "The popup goes away and the floppy is ejected."
	:x                      400
	:y                      330
	:width                  105
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Cancel & Eject"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         eject
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (unformatted_popup format_eject)
		:when                   (Notify )
		:to                     (unformatted_popup format_eject)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (eject)
		)
	)
)
)

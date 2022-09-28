;GIL-3
; @(#)radio.G	1.12	91/10/24 SMI
(
(
	:type                   :menu
	:name                   ActiveMenu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Active Stations"
	:menu-item-labels       ()
	:menu-item-label-types  ()
	:menu-item-states       ()
	:menu-item-defaults     ()
	:initial-selections     ()
	:menu-item-handlers     ()
	:menu-item-menus        ()
	:menu-item-colors       ()
	:pinnable               t
	:user-data              ()
	:actions                ()
)
(
	:type                   :base-window
	:name                   RadioFrame
	:owner                  nil
	:width                  418
	:height                 151
	:background-color       ""
	:foreground-color       ""
	:label                  "Radio Free Ethernet Receiver"
	:label-type             :string
	:initial-state          :open
	:show-footer            t
	:resizable              nil
	:icon-file              "radio_off.icon"
	:icon-label             ""
	:icon-mask-file         "radio_off.mask.icon"
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   RadioControls
	:owner                  RadioFrame
	:help                   "The Radio Free Ethernet receiver program scans the network for broadcast audio data and tunes in a particular station.  For an overview of Radio Free Ethernet, please refer to the radio(6) manual page."
	:x                      0
	:y                      0
	:width                  418
	:height                 88
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            t
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   StatusMessage
	:owner                  RadioControls
	:help                   "The current state of the Radio Free Ethernet receiver is displayed in this status field."
	:x                      165
	:y                      6
	:width                  89
	:height                 13
	:foreground-color       ""
	:label                  "No Broadcast"
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   StatusGroup
	:x                      165
	:y                      6
	:group-type             :row
	:members                (StatusMessage )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :vertical-centers
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          RadioControls
	:anchor-point           :north
	:reference-point        :north
	:horizontal-offset      0
	:vertical-offset        6
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   VolumeButton
	:owner                  RadioControls
	:help                   "The Volume... button starts up an Audio Control Panel that allows the play volume to be set.  (For more information, see the gaintool(6) manual page)."
	:x                      8
	:y                      16
	:width                  73
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Volume..."
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         volume_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (RadioFrame VolumeButton)
		:when                   (Notify )
		:to                     (RadioFrame VolumeButton)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (volume_notify)
		)
	)
)
(
	:type                   :button
	:name                   PropsButton
	:owner                  RadioControls
	:help                   "The Props... button brings up a tool property sheet that allows various program parameters to be set."
	:x                      85
	:y                      16
	:width                  61
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Props..."
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (RadioFrame PropsButton)
		:when                   (Notify )
		:to                     (PropsFrame)
		:function_type          :user_defined
		:arg_type               ()
		:action                 (Show)
		)
	)
)
(
	:type                   :group
	:name                   LeftButtonGroup
	:x                      8
	:y                      16
	:group-type             :row
	:members                (VolumeButton PropsButton )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :left-edges
	:horizontal-spacing     4
	:vertical-spacing       10
	:anchor-object          RadioControls
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      8
	:vertical-offset        16
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   PowerButton
	:owner                  RadioControls
	:help                   ""
	:x                      340
	:y                      16
	:width                  54
	:height                 23
	:value-x                340
	:value-y                16
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :nonexclusive
	:selection-required     t
	:label                  ""
	:label-type             :string
	:notify-handler         power_notify
	:event-handler          nil
	:choices                ("Power" )
	:choice-label-types     (:string )
	:choice-colors          ("" )
	:initial-selections     (nil )
	:initial-state          :active
	:user-data              ()
	:actions                (
		(
		:from                   (RadioFrame PowerButton)
		:when                   (Notify )
		:to                     (RadioFrame PowerButton)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (power_notify)
		)
	)
)
(
	:type                   :group
	:name                   PowerGroup
	:x                      340
	:y                      16
	:group-type             :row
	:members                (PowerButton )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :left-edges
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          RadioControls
	:anchor-point           :north-east
	:reference-point        :north-east
	:horizontal-offset      -24
	:vertical-offset        16
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   ScanButton
	:owner                  RadioControls
	:help                   "The Scan button clears the current station and starts the receiver scanning for a new station.  If multiple stations are broadcasting, this button may be pressed repeatedly to browse through them."
	:x                      186
	:y                      35
	:width                  46
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Scan"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         scan_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (RadioFrame ScanButton)
		:when                   (Notify )
		:to                     (RadioFrame ScanButton)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (scan_notify)
		)
	)
)
(
	:type                   :group
	:name                   ScanGroup
	:x                      186
	:y                      35
	:group-type             :column
	:members                (ScanButton )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :vertical-centers
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          RadioControls
	:anchor-point           :center
	:reference-point        :center
	:horizontal-offset      0
	:vertical-offset        0
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   StationButton
	:owner                  RadioControls
	:help                   "The Station item brings up a menu containing a list of the currently active radio stations.  Selecting a station from this menu will tune in that station.  Station names are enclosed in square brackets if the station is determined to be on-the-air but has not broadcast any data for some time.  This can occur when the transmitter squelches silent audio input."
	:x                      9
	:y                      61
	:width                  79
	:height                 15
	:constant-width         nil
	:button-type            :abbreviated
	:foreground-color       ""
	:label                  "Station:"
	:label-type             :string
	:initial-state          :inactive
	:menu                   ActiveMenu
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   StationButtonText
	:owner                  RadioControls
	:help                   "The Station item brings up a menu containing a list of the currently active radio stations.  Selecting a station from this menu will tune in that station.  Station names are enclosed in square brackets if the station is determined to be on-the-air but has not broadcast any data for some time.  This can occur when the transmitter squelches silent audio input."
	:x                      96
	:y                      61
	:width                  48
	:height                 15
	:value-x                96
	:value-y                61
	:value-length           6
	:stored-length          6
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  ""
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       nil
	:initial-value          ""
	:initial-state          :active
	:read-only              t
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   StationGroup
	:x                      9
	:y                      61
	:group-type             :row
	:members                (StationButton StationButtonText )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :vertical-centers
	:horizontal-spacing     8
	:vertical-spacing       10
	:anchor-object          nil
	:anchor-point           :west
	:reference-point        :west
	:horizontal-offset      0
	:vertical-offset        0
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   DJText
	:owner                  RadioControls
	:help                   "The DJ field displays the name of the disc jockey for the radio station currently being received.  If the field is blank, it is because a station identification packet has not yet been processed for the current station."
	:x                      150
	:y                      61
	:width                  267
	:height                 15
	:value-x                177
	:value-y                61
	:value-length           30
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "DJ:"
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       nil
	:initial-value          ""
	:initial-state          :invisible
	:read-only              t
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   StationIdGroup
	:x                      9
	:y                      61
	:group-type             :row
	:members                (StationGroup DJText )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :vertical-centers
	:horizontal-spacing     6
	:vertical-spacing       6
	:anchor-object          RadioControls
	:anchor-point           :south
	:reference-point        :south
	:horizontal-offset      4
	:vertical-offset        -12
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   PresetControls
	:owner                  RadioFrame
	:help                   "There are six radio preset buttons that may be programmed (using the Set and Clear buttons) to identify particular radio station names.  When a preset button is pressed, the program attempts to tune in the named station."
	:x                      0
	:y                      88
	:width                  418
	:height                 63
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            t
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   PresetButton1
	:owner                  PresetControls
	:help                   "There are six radio preset buttons that may be programmed (using the Set and Clear buttons) to identify particular radio station names.  When a preset button is pressed, the program attempts to tune in the named station."
	:x                      13
	:y                      8
	:width                  44
	:height                 19
	:constant-width         t
	:button-type            :normal
	:foreground-color       ""
	:label                  " "
	:label-type             :string
	:initial-state          :inactive
	:menu                   nil
	:notify-handler         preset_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (RadioFrame PresetButton1)
		:when                   (Notify )
		:to                     (RadioFrame PresetButton1)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (preset_notify)
		)
	)
)
(
	:type                   :button
	:name                   PresetButton2
	:owner                  PresetControls
	:help                   "There are six radio preset buttons that may be programmed (using the Set and Clear buttons) to identify particular radio station names.  When a preset button is pressed, the program attempts to tune in the named station."
	:x                      79
	:y                      8
	:width                  44
	:height                 19
	:constant-width         t
	:button-type            :normal
	:foreground-color       ""
	:label                  " "
	:label-type             :string
	:initial-state          :inactive
	:menu                   nil
	:notify-handler         preset_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (RadioFrame PresetButton2)
		:when                   (Notify )
		:to                     (RadioFrame PresetButton2)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (preset_notify)
		)
	)
)
(
	:type                   :button
	:name                   PresetButton3
	:owner                  PresetControls
	:help                   "There are six radio preset buttons that may be programmed (using the Set and Clear buttons) to identify particular radio station names.  When a preset button is pressed, the program attempts to tune in the named station."
	:x                      145
	:y                      8
	:width                  44
	:height                 19
	:constant-width         t
	:button-type            :normal
	:foreground-color       ""
	:label                  " "
	:label-type             :string
	:initial-state          :inactive
	:menu                   nil
	:notify-handler         preset_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (RadioFrame PresetButton3)
		:when                   (Notify )
		:to                     (RadioFrame PresetButton3)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (preset_notify)
		)
	)
)
(
	:type                   :button
	:name                   PresetButton4
	:owner                  PresetControls
	:help                   "There are six radio preset buttons that may be programmed (using the Set and Clear buttons) to identify particular radio station names.  When a preset button is pressed, the program attempts to tune in the named station."
	:x                      211
	:y                      8
	:width                  44
	:height                 19
	:constant-width         t
	:button-type            :normal
	:foreground-color       ""
	:label                  " "
	:label-type             :string
	:initial-state          :inactive
	:menu                   nil
	:notify-handler         preset_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (RadioFrame PresetButton4)
		:when                   (Notify )
		:to                     (RadioFrame PresetButton4)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (preset_notify)
		)
	)
)
(
	:type                   :button
	:name                   PresetButton5
	:owner                  PresetControls
	:help                   "There are six radio preset buttons that may be programmed (using the Set and Clear buttons) to identify particular radio station names.  When a preset button is pressed, the program attempts to tune in the named station."
	:x                      277
	:y                      8
	:width                  44
	:height                 19
	:constant-width         t
	:button-type            :normal
	:foreground-color       ""
	:label                  " "
	:label-type             :string
	:initial-state          :inactive
	:menu                   nil
	:notify-handler         preset_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (RadioFrame PresetButton5)
		:when                   (Notify )
		:to                     (RadioFrame PresetButton5)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (preset_notify)
		)
	)
)
(
	:type                   :button
	:name                   PresetButton6
	:owner                  PresetControls
	:help                   "There are six radio preset buttons that may be programmed (using the Set and Clear buttons) to identify particular radio station names.  When a preset button is pressed, the program attempts to tune in the named station."
	:x                      343
	:y                      8
	:width                  44
	:height                 19
	:constant-width         t
	:button-type            :normal
	:foreground-color       ""
	:label                  " "
	:label-type             :string
	:initial-state          :inactive
	:menu                   nil
	:notify-handler         preset_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (RadioFrame PresetButton6)
		:when                   (Notify )
		:to                     (RadioFrame PresetButton6)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (preset_notify)
		)
	)
)
(
	:type                   :group
	:name                   PresetButtonGroup
	:x                      13
	:y                      8
	:group-type             :row
	:members                (PresetButton1 PresetButton2 PresetButton3 PresetButton4 PresetButton5 PresetButton6 )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :left-edges
	:horizontal-spacing     4
	:vertical-spacing       10
	:anchor-object          PresetControls
	:anchor-point           :north
	:reference-point        :north
	:horizontal-offset      0
	:vertical-offset        8
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   SetPresetButton
	:owner                  PresetControls
	:help                   "The Set button sets the preset whose name is entered in the Preset Station field.  If the Preset Station is blank, it sets the preset whose name matches the radio station that is currently playing, if any.  It also causes the current program parameters and presets to be written out to the initialization file ($HOME/.radiorc)."
	:x                      174
	:y                      38
	:width                  37
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Set"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         set_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (RadioFrame SetPresetButton)
		:when                   (Notify )
		:to                     (RadioFrame SetPresetButton)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (set_notify)
		)
	)
)
(
	:type                   :button
	:name                   ClearPresetButton
	:owner                  PresetControls
	:help                   "The Clear button clears the preset whose name is entered in the Preset Station field.  If the Preset Station is blank, it clears the preset whose name matches the radio station that is currently playing, if any.  It also causes the current program parameters and presets to be written out to the initialization file ($HOME/.radiorc)."
	:x                      221
	:y                      38
	:width                  50
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Clear"
	:label-type             :string
	:initial-state          :inactive
	:menu                   nil
	:notify-handler         clear_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (RadioFrame ClearPresetButton)
		:when                   (Notify )
		:to                     (RadioFrame ClearPresetButton)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (clear_notify)
		)
	)
)
(
	:type                   :text-field
	:name                   PresetItem
	:owner                  PresetControls
	:help                   "The Preset Station field is used to type in a four-character station name that may be used to Set or Clear one of the six station preset buttons."
	:x                      10
	:y                      40
	:width                  154
	:height                 15
	:value-x                116
	:value-y                40
	:value-length           6
	:stored-length          4
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Preset Station:"
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
	:type                   :group
	:name                   PresetStationGroup
	:x                      10
	:y                      38
	:group-type             :row
	:members                (PresetItem SetPresetButton ClearPresetButton )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :left-edges
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          PresetControls
	:anchor-point           :south-west
	:reference-point        :south-west
	:horizontal-offset      10
	:vertical-offset        -6
	:user-data              ()
	:actions                ()
)
(
	:type                   :popup-window
	:name                   PropsFrame
	:owner                  RadioFrame
	:width                  418
	:height                 130
	:background-color       ""
	:foreground-color       ""
	:label                  "Radio Free Ethernet:  Properties"
	:label-type             :string
	:initial-state          :invisible
	:show-footer            t
	:resizable              nil
	:pinned                 nil
	:done-handler           nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   PropsControls
	:owner                  PropsFrame
	:help                   "For information on particular tool properties, position the mouse over the property in question and press the Help key."
	:x                      0
	:y                      0
	:width                  418
	:height                 130
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            t
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   AutoScanItem
	:owner                  PropsControls
	:help                   "When the Auto Scan feature is enabled, the radio tuner will scan for a new radio station when the current station goes off the air or is quiet for the specified amount of time."
	:x                      103
	:y                      8
	:width                  116
	:height                 23
	:value-x                181
	:value-y                8
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :check
	:selection-required     t
	:label                  "Auto Scan:"
	:label-type             :string
	:notify-handler         autoscan_notify
	:event-handler          nil
	:choices                (" " )
	:choice-label-types     (:string )
	:choice-colors          ("" )
	:initial-selections     (t )
	:initial-state          :active
	:user-data              ()
	:actions                (
		(
		:from                   (PropsFrame AutoScanItem)
		:when                   (Notify )
		:to                     (PropsFrame AutoScanItem)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (autoscan_notify)
		)
	)
)
(
	:type                   :text-field
	:name                   ScanDelayItem
	:owner                  PropsControls
	:help                   "When the Auto Scan feature is enabled, the radio tuner will scan for a new radio station when the current station goes off the air or is quiet for the specified amount of time."
	:x                      219
	:y                      12
	:width                  115
	:height                 15
	:value-x                263
	:value-y                12
	:value-length           3
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :numeric
	:label                  "after:"
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       t
	:max-value              600
	:min-value              0
	:initial-value          60
	:initial-state          :active
	:read-only              nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   SecondsMessage
	:owner                  PropsControls
	:help                   "When the Auto Scan feature is enabled, the radio tuner will scan for a new radio station when the current station goes off the air or is quiet for the specified amount of time."
	:x                      338
	:y                      13
	:width                  56
	:height                 13
	:foreground-color       ""
	:label                  "seconds"
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   ScanDelayGroup
	:x                      219
	:y                      12
	:group-type             :row
	:members                (ScanDelayItem SecondsMessage )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :labels
	:horizontal-spacing     4
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
	:name                   AutoScanGroup
	:x                      103
	:y                      8
	:group-type             :row
	:members                (AutoScanItem ScanDelayGroup )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :vertical-centers
	:horizontal-spacing     0
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
	:type                   :setting
	:name                   AutoReleaseItem
	:owner                  PropsControls
	:help                   "When the Allow Device Release feature is enabled, the radio tuner will relinquish the audio output device whenever another process tries to use it.  This allows other applications, such as AudioTool, the opportunity to interrupt the playing of a radio station in order to play what is presumed to be more important audio data."
	:x                      24
	:y                      33
	:width                  195
	:height                 23
	:value-x                181
	:value-y                33
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :check
	:selection-required     t
	:label                  "Allow Device Release:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                (" " )
	:choice-label-types     (:string )
	:choice-colors          ("" )
	:initial-selections     (t )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   PropsChoiceGroup
	:x                      24
	:y                      8
	:group-type             :column
	:members                (AutoScanGroup AutoReleaseItem )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       2
	:anchor-object          nil
	:anchor-point           :north
	:reference-point        :north
	:horizontal-offset      0
	:vertical-offset        8
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   OutputItem
	:owner                  PropsControls
	:help                   "The Audio Output field is used to specify the audio device on which the current radio station transmission is played.  Ordinarily, this field should be set to \"/dev/audio\" to play the station on the workstation speaker."
	:x                      79
	:y                      63
	:width                  278
	:height                 15
	:value-x                181
	:value-y                63
	:value-length           22
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Audio Output:"
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
	:type                   :group
	:name                   PropsAllGroup
	:x                      24
	:y                      8
	:group-type             :column
	:members                (PropsChoiceGroup OutputItem )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       7
	:anchor-object          PropsControls
	:anchor-point           :north
	:reference-point        :north
	:horizontal-offset      0
	:vertical-offset        8
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   ApplyButton
	:owner                  PropsControls
	:help                   "The Apply button causes the current property sheet settings to take effect for the program.  It also  causes the current program parameters and presets to be written out to the initialization file ($HOME/.radiorc)."
	:x                      152
	:y                      103
	:width                  53
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Apply"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         apply_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (PropsFrame ApplyButton)
		:when                   (Notify )
		:to                     (PropsFrame ApplyButton)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (apply_notify)
		)
	)
)
(
	:type                   :button
	:name                   ResetButton
	:owner                  PropsControls
	:help                   "The Reset button resets the property sheet settings to the current program configuration."
	:x                      215
	:y                      103
	:width                  51
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Reset"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         reset_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (PropsFrame ResetButton)
		:when                   (Notify )
		:to                     (PropsFrame ResetButton)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (reset_notify)
		)
	)
)
(
	:type                   :group
	:name                   ApplyResetGroup
	:x                      152
	:y                      103
	:group-type             :row
	:members                (ApplyButton ResetButton )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :left-edges
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          PropsControls
	:anchor-point           :south
	:reference-point        :south
	:horizontal-offset      0
	:vertical-offset        -8
	:user-data              ()
	:actions                ()
)
)

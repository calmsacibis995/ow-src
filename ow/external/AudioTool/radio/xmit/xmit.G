;GIL-3
; @(#)xmit.G	1.11	91/11/15 SMI
(
(
	:type                   :menu
	:name                   StationMenu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Station List"
	:menu-item-labels       (" " "Edit..." )
	:menu-item-label-types  (:string :string )
	:menu-item-states       (:inactive :active )
	:menu-item-defaults     (nil t )
	:initial-selections     (nil nil )
	:menu-item-handlers     (nil nil )
	:menu-item-menus        (nil nil )
	:menu-item-colors       ("" "" )
	:pinnable               nil
	:user-data              ()
	:actions                (
		(
		:from                   (StationMenu "Edit...")
		:when                   (Notify )
		:to                     (StationFrame)
		:function_type          :user_defined
		:arg_type               ()
		:action                 (Show)
		)
	)
)
(
	:type                   :base-window
	:name                   XmitFrame
	:owner                  nil
	:width                  340
	:height                 108
	:background-color       ""
	:foreground-color       ""
	:label                  "Radio Free Ethernet Transmitter"
	:label-type             :string
	:initial-state          :open
	:show-footer            t
	:resizable              nil
	:icon-file              "xmit_off.icon"
	:icon-label             ""
	:icon-mask-file         "xmit_off.mask.icon"
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   XmitControlPanel
	:owner                  XmitFrame
	:help                   "The Radio Free Ethernet transmitter program broadcasts audio data over the network.  For an overview of Radio Free Ethernet, please refer to the radio(6) manual page."
	:x                      0
	:y                      0
	:width                  340
	:height                 108
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
	:owner                  XmitControlPanel
	:help                   "The current state of the Radio Free Ethernet transmitter is displayed in this status field."
	:x                      126
	:y                      6
	:width                  89
	:height                 13
	:foreground-color       ""
	:label                  "Broadcasting"
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
	:x                      126
	:y                      6
	:group-type             :row
	:members                (StatusMessage )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :left-edges
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          XmitControlPanel
	:anchor-point           :north
	:reference-point        :north
	:horizontal-offset      0
	:vertical-offset        6
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   PowerButton
	:owner                  XmitControlPanel
	:help                   "The Power button toggles the radio transmitter
off and on.  While the transmitter is turned on, audio data is broadcast from your machine over the network.  A station name must be set before the transmitter can be enabled."
	:x                      262
	:y                      21
	:width                  54
	:height                 23
	:value-x                262
	:value-y                21
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
	:initial-state          :inactive
	:user-data              ()
	:actions                (
		(
		:from                   (XmitFrame PowerButton)
		:when                   (Notify )
		:to                     (XmitFrame PowerButton)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (power_notify)
		)
	)
)
(
	:type                   :group
	:name                   PowerGroup
	:x                      262
	:y                      21
	:group-type             :row
	:members                (PowerButton )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          XmitControlPanel
	:anchor-point           :north-east
	:reference-point        :north-east
	:horizontal-offset      -24
	:vertical-offset        21
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   StationButton
	:owner                  XmitControlPanel
	:help                   "The Station item brings up a menu containing a list of the currently configured broadcast stations.  Selecting a station from this list prepares the transmitter to begin broadcasting, using the station name as an identifying string.  The Edit... menu item brings up a panel that may be used to edit the broadcast station list and parameters."
	:x                      8
	:y                      26
	:width                  79
	:height                 15
	:constant-width         nil
	:button-type            :abbreviated
	:foreground-color       ""
	:label                  "Station:"
	:label-type             :string
	:initial-state          :active
	:menu                   StationMenu
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   StationButtonText
	:owner                  XmitControlPanel
	:help                   "The Station item brings up a menu containing a list of the currently configured broadcast stations.  Selecting a station from this list prepares the transmitter to begin broadcasting, using the station name as an identifying string.  The Edit... menu item brings up a panel that may be used to edit the broadcast station list and parameters."
	:x                      91
	:y                      26
	:width                  48
	:height                 15
	:value-x                91
	:value-y                26
	:value-length           6
	:stored-length          6
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  ""
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       nil
	:initial-value          "(none)"
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
	:x                      8
	:y                      26
	:group-type             :row
	:members                (StationButton StationButtonText )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :labels
	:horizontal-spacing     4
	:vertical-spacing       10
	:anchor-object          XmitControlPanel
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      8
	:vertical-offset        26
	:user-data              ()
	:actions                ()
)
(
	:type                   :slider
	:name                   VolumeControl
	:owner                  XmitControlPanel
	:help                   "The Input Volume slider may be used to manualy adjust the audio input gain level.  It is automatically adjusted when Auto Volume Adjust is enabled."
	:x                      50
	:y                      63
	:width                  262
	:height                 20
	:value-x                149
	:value-y                63
	:slider-width           100
	:ticks                  0
	:foreground-color       ""
	:label                  "Input Volume:"
	:label-type             :string
	:layout-type            :horizontal
	:orientation            :horizontal
	:show-endboxes          nil
	:show-range             nil
	:show-value             t
	:min-value              0
	:max-value              100
	:min-value-string       ""
	:max-value-string       ""
	:min-tick-string        ""
	:max-tick-string        ""
	:initial-value          0
	:initial-state          :inactive
	:notify-handler         volume_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (XmitFrame VolumeControl)
		:when                   (Notify )
		:to                     (XmitFrame VolumeControl)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (volume_notify)
		)
	)
)
(
	:type                   :setting
	:name                   AgcControl
	:owner                  XmitControlPanel
	:help                   "Auto Volume Adjust enables an automatic gain control (AGC) algorithm that monitors the audio input volume and adjusts it when the data is too soft or too loud.  The Input Volume slider will move when the recording gain is adjusted."
	:x                      8
	:y                      79
	:width                  180
	:height                 23
	:value-x                154
	:value-y                79
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :check
	:selection-required     t
	:label                  "Auto Volume Adjust:"
	:label-type             :string
	:notify-handler         agc_notify
	:event-handler          nil
	:choices                ("" )
	:choice-label-types     (:string )
	:choice-colors          ("" )
	:initial-selections     (t )
	:initial-state          :inactive
	:user-data              ()
	:actions                (
		(
		:from                   (XmitFrame AgcControl)
		:when                   (Notify )
		:to                     (XmitFrame AgcControl)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (agc_notify)
		)
	)
)
(
	:type                   :group
	:name                   VolumeGroup
	:x                      8
	:y                      63
	:group-type             :column
	:members                (VolumeControl AgcControl )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     -2
	:vertical-spacing       -4
	:anchor-object          XmitControlPanel
	:anchor-point           :south-west
	:reference-point        :south-west
	:horizontal-offset      8
	:vertical-offset        -6
	:user-data              ()
	:actions                ()
)
(
	:type                   :popup-window
	:name                   StationFrame
	:owner                  XmitFrame
	:width                  294
	:height                 350
	:background-color       ""
	:foreground-color       ""
	:label                  "Radio Free Ethernet:  Stations"
	:label-type             :string
	:initial-state          :invisible
	:show-footer            t
	:resizable              nil
	:pinned                 nil
	:done-handler           nil
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (StationFrame)
		:when                   (Resize )
		:to                     (StationFrame)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (station_resize)
		)
	)
)
(
	:type                   :control-area
	:name                   StationControlPanel
	:owner                  StationFrame
	:help                   "For information on particular broadcast parameters, position the mouse over the item in question and press the Help key."
	:x                      0
	:y                      0
	:width                  294
	:height                 350
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
	:type                   :scrolling-list
	:name                   StationList
	:owner                  StationControlPanel
	:help                   "This is a list of the currently configured broadcast stations.  Selecting an entry in this list will update the panel to display the station parameters.  Selected stations may be changed or deleted."
	:x                      38
	:y                      10
	:width                  200
	:height                 74
	:value-x                38
	:value-y                10
	:rows                   3
	:foreground-color       ""
	:label                  ""
	:title                  ""
	:label-type             :string
	:layout-type            :horizontal
	:read-only              t
	:multiple-selections    nil
	:selection-required     nil
	:initial-state          :active
	:droppable              nil
	:default-drop-site      nil
	:menu                   nil
	:notify-handler         list_notify
	:event-handler          nil
	:initial-list-values    ()
	:initial-list-glyphs    ()
	:initial-selections     ()
	:user-data              ()
	:actions                (
		(
		:from                   (StationFrame StationList)
		:when                   (Notify )
		:to                     (StationFrame StationList)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (list_notify)
		)
	)
)
(
	:type                   :group
	:name                   StationListGroup
	:x                      38
	:y                      10
	:group-type             :column
	:members                (StationList )
	:rows                   0
	:row-alignment          :top-edges
	:columns                1
	:column-alignment       :vertical-centers
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          StationControlPanel
	:anchor-point           :north
	:reference-point        :north
	:horizontal-offset      0
	:vertical-offset        10
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   StationItem
	:owner                  StationControlPanel
	:help                   "The Station item is used to enter a station name (up to four characters).  The station name may be thought of as the radio station call letters, and is used to identify radio broadcast packets."
	:x                      65
	:y                      96
	:width                  107
	:height                 15
	:value-x                124
	:value-y                96
	:value-length           6
	:stored-length          4
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Station:"
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
	:name                   SignonItem
	:owner                  StationControlPanel
	:help                   "The Sign-On File item specifies the an audio file that will be broadcast when the transmitter is initially powered on.  Once the Sign-On File has been completely broadcast, the transmitter switches to the Audio Input file or device."
	:x                      26
	:y                      129
	:width                  258
	:height                 15
	:value-x                124
	:value-y                129
	:value-length           20
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Sign-On File:"
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
	:name                   InputItem
	:owner                  StationControlPanel
	:help                   "The Audio Input item specifies the source of audio data for normal station broadcasting.  An audio file or device name may be specified.  The default setting (/dev/audio) enables data to be read from the workstation microphone input and broadcast over the network."
	:x                      33
	:y                      150
	:width                  251
	:height                 15
	:value-x                124
	:value-y                150
	:value-length           20
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Audio Input:"
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
	:name                   SignoffItem
	:owner                  StationControlPanel
	:help                   "The Sign-Off File item specifies the an audio file that will be broadcast when the transmitter is powered off."
	:x                      23
	:y                      171
	:width                  261
	:height                 15
	:value-x                124
	:value-y                171
	:value-length           20
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Sign-Off File:"
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
	:name                   FileItemGroup
	:x                      23
	:y                      129
	:group-type             :column
	:members                (SignonItem InputItem SignoffItem )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       6
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
	:name                   ShutoffControl
	:owner                  StationControlPanel
	:help                   "Auto-Shutoff determines the action to be taken when an audio broadcast has been silent for a full minute.  When enabled, the station will automatically sign off.  This feature allows the transmitter to shut down at the end of a CD or tape input."
	:x                      23
	:y                      204
	:width                  135
	:height                 23
	:value-x                124
	:value-y                204
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :check
	:selection-required     t
	:label                  "Auto-Shutoff:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("" )
	:choice-label-types     (:string )
	:choice-colors          ("" )
	:initial-selections     (t )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   FormatControl
	:owner                  StationControlPanel
	:help                   "The Audio Format item selects the data format for audio packets transmitted over the network.  The Uncompressed audio format is 8000 bytes/sec, while the Compressed audio format is 4000 bytes/sec.  The Audio Format setting applies only to audio input from the audio device (/dev/audio)."
	:x                      23
	:y                      225
	:width                  226
	:height                 23
	:value-x                124
	:value-y                225
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :stack
	:selection-required     t
	:label                  "Audio Format:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("Uncompressed" "Compressed" )
	:choice-label-types     (:string :string )
	:choice-colors          ("" "" )
	:choice-defaults        (nil t )
	:initial-selections     (nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   AudioGroup
	:x                      23
	:y                      204
	:group-type             :column
	:members                (ShutoffControl FormatControl )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       -2
	:anchor-object          nil
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      0
	:vertical-offset        0
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   AddressItem
	:owner                  StationControlPanel
	:help                   "The Multicast Addr item may be used to specify a particular broadcast address.  If a hostname is supplied, it must refer to an entry in the hosts table that contains a multicast address (you cannot, for instance, broadcast to a particular machine by name).  This item is enabled only when the Broadcast Band is set to `as indicated'."
	:x                      14
	:y                      266
	:width                  270
	:height                 15
	:value-x                124
	:value-y                266
	:value-length           20
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Multicast Addr:"
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
	:name                   HopcountItem
	:owner                  StationControlPanel
	:help                   "The Multicast Hops item is used to specify the number of network gateways over which a broadcast may be relayed.  A hopcount of 1 ensures that the broadcast will not go beyond the local subnet.

This value is ignored if the Multicast Addr field is set to BROADCAST, as UDP Broadcast packets are not relayed over gateways."
	:x                      10
	:y                      287
	:width                  185
	:height                 15
	:value-x                124
	:value-y                287
	:value-length           2
	:stored-length          2
	:rows                   3
	:foreground-color       ""
	:text-type              :numeric
	:label                  "Multicast Hops:"
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       t
	:max-value              99
	:min-value              1
	:initial-value          0
	:initial-state          :active
	:read-only              nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   HopcountMessage
	:owner                  StationControlPanel
	:help                   ""
	:x                      199
	:y                      287
	:width                  64
	:height                 13
	:foreground-color       ""
	:label                  "gateways"
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   HopGroup
	:x                      10
	:y                      287
	:group-type             :row
	:members                (HopcountItem HopcountMessage )
	:rows                   1
	:row-alignment          :top-edges
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
	:name                   MulticastGroup
	:x                      10
	:y                      266
	:group-type             :column
	:members                (AddressItem HopGroup )
	:rows                   0
	:row-alignment          :top-edges
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       6
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
	:name                   AdvancedGroup
	:x                      10
	:y                      129
	:group-type             :column
	:members                (FileItemGroup AudioGroup MulticastGroup )
	:rows                   0
	:row-alignment          :top-edges
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       18
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
	:name                   StationConfigGroup
	:x                      10
	:y                      96
	:group-type             :column
	:members                (StationItem AdvancedGroup )
	:rows                   0
	:row-alignment          :top-edges
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       18
	:anchor-object          StationListGroup
	:anchor-point           :south
	:reference-point        :north
	:horizontal-offset      0
	:vertical-offset        12
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   AddButton
	:owner                  StationControlPanel
	:help                   "The Add button adds a station to the station list with the current parameter settings.  If the named station is already in the list, its parameters are updated.  A station name of up to four letters must be supplied in the Station: item.

The Add button also causes the current program parameters to be written out to the initialization file ($HOME/.radiorc)."
	:x                      62
	:y                      321
	:width                  42
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Add"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         add_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (StationFrame AddButton)
		:when                   (Notify )
		:to                     (StationFrame AddButton)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (add_notify)
		)
	)
)
(
	:type                   :button
	:name                   ChangeButton
	:owner                  StationControlPanel
	:help                   "The Change button updates the selected station with the current parameter settings.  If no station is selected, the named station is added or updated.  Changes to the parameters for a station that is currently broadcasting will not take effect until the transmitter is turned off.

The Change button also causes the current program parameters to be written out to the initialization file ($HOME/.radiorc)."
	:x                      108
	:y                      321
	:width                  64
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Change"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         change_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (StationFrame ChangeButton)
		:when                   (Notify )
		:to                     (StationFrame ChangeButton)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (change_notify)
		)
	)
)
(
	:type                   :button
	:name                   DeleteButton
	:owner                  StationControlPanel
	:help                   "The Delete button removes the selected station from the station list.  It also causes the current program parameters to be written out to the initialization file ($HOME/.radiorc)."
	:x                      176
	:y                      321
	:width                  57
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Delete"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         delete_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (StationFrame DeleteButton)
		:when                   (Notify )
		:to                     (StationFrame DeleteButton)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (delete_notify)
		)
	)
)
(
	:type                   :group
	:name                   ChangeButtonGroup
	:x                      62
	:y                      321
	:group-type             :row
	:members                (AddButton ChangeButton DeleteButton )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :labels
	:horizontal-spacing     4
	:vertical-spacing       10
	:anchor-object          StationControlPanel
	:anchor-point           :south
	:reference-point        :south
	:horizontal-offset      0
	:vertical-offset        -10
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   PlusMinusButton
	:owner                  StationControlPanel
	:help                   "The +/- button toggles between the default and advanced modes of the Station Configuration panel."
	:x                      262
	:y                      321
	:width                  26
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "-"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         plusminus_notify
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (StationFrame PlusMinusButton)
		:when                   (Notify )
		:to                     (StationFrame PlusMinusButton)
		:function_type          CallFunction
		:arg_type               ()
		:action                 (plusminus_notify)
		)
	)
)
(
	:type                   :group
	:name                   PlusMinusGroup
	:x                      262
	:y                      321
	:group-type             :row
	:members                (PlusMinusButton )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          StationControlPanel
	:anchor-point           :south-east
	:reference-point        :south-east
	:horizontal-offset      -6
	:vertical-offset        -10
	:user-data              ()
	:actions                ()
)
)

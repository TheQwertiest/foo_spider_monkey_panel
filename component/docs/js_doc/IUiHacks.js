/** @constructor */
function IUIHacks() {
    /** @type{IAero} */
    this.Aero = undefined;

    /** @type{boolean} */
    this.BlockMaximize = undefined;

    /** @type{boolean} */
    this.DisableSizing = undefined;

    /** @type{boolean} */
    this.EnableSizing = undefined;

    /** @type{float} */
    this.FoobarCPUUsage = undefined;

    /**
     * 0 - Default;
     * 1 - SmallCaption;
     * 2 - NoCaption;
     * 3 - NoBorder;
     *
     * @type{number}
     */
    this.FrameStyle = undefined;

    /** @type{boolean} */
    this.FullScreen = undefined;

    /**
     * 0 - Show;
     * 1 - Hide;
     * 2 - Auto;
     *
     * @type{number}
     */
    this.MainMenuState = undefined;

    /** @type{number} */
    this.MainWindowID = undefined;

    /**
     * 0 - Normal;
     * 1 - Minimized;
     * 2 - Maximized;
     *
     * @type{number}
     */
    this.MainWindowState = undefined;

    /** @type {IMasterVolume} */
    this.MasterVolume = undefined;

    /** @type{boolean|IConstraints} */
    this.MaxSize = undefined;

    /** @type{boolean|IConstraints} */
    this.MinSize = undefined;

    /**
     * 0 - Default;
     * 1 - Middle mouse button only;
     * 2 - Left mouse button only;
     * 3 - Left or middle mouse button;
     *
     * @type{number}
     */
    this.MoveStyle = undefined;

    /** @type{boolean} */
    this.StatusBarState = undefined;

    /** @type{float} */
    this.SystemCPUUsage = undefined;

    /** @constructor */
    function IAero() {

        /** @type{boolean} */
        this.Active = undefined;

        /**
         *  0 - Default;
         *  1 - Disabled;
         *  2 - GlassFrame;
         *  3 - SheetOfGlass: aero effect applied on whole window;
         *
         *  @type{number}
         */
        this.Effect = undefined;

        /** @type{number} */
        this.Top = undefined;

        /** @type{number} */
        this.Right = undefined;

        /** @type{number} */
        this.Left = undefined;

        /** @type{number} */
        this.Bottom = undefined;

    }

    /** @constructor */
    function IConstraints() {

        /** @type{number} */
        this.Width = undefined;
        /** @type{number} */
        this.Height = undefined;

    }

    /** @constructor */
    function IMasterVolume() {
        /** @type{number} */
        this.ChannelCount = undefined; // read-only

        /** @type{number} */
        this.Mute = undefined;

        /** @type{float} */
        this.Volume = undefined;

        /**
         * @param {number} channelIdx
         * @return {float}
         */
        this.GetChannelVolume = function(channelIdx) {};

        /**
         * @param {number} channelIdx
         * @param {float} volume
         */
        this.SetChannelVolume = function(channelIdx, volume) {};

        this.VolumeStepDown = function() {};

        this.VolumeStepUp = function() {};
    }

    /**
     * @param {number} x
     * @param {number} y
     * @param {number} w
     * @param {number} h
     */
    this.SetPseudoCaption = function (x, y, w, h) {};
}

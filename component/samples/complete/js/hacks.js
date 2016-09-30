_.mixin({
	hacks : function () {
		this.disable = function () {
			UIHacks.MainMenuState = this.MainMenuState.Show;
			UIHacks.FrameStyle = this.FrameStyle.Default;
			UIHacks.StatusBarState = true;
		}
		
		this.enable = function () {
			UIHacks.MainMenuState = this.MainMenuState.Hide;
			UIHacks.FrameStyle = this.FrameStyle.NoBorder;
			UIHacks.StatusBarState = false;
		}
		
		this.set_caption = function (x, y, w, h) {
			UIHacks.SetPseudoCaption(x, y, w, h);
		}
		
		this.MainMenuState = { Show : 0, Hide : 1, Auto : 2 };
		this.FrameStyle = { Default : 0, SmallCaption : 1, NoCaption : 2, NoBorder : 3 };
		this.MoveStyle = { Default : 0, Middle : 1, Left : 2, Both : 3 };
		
		UIHacks = new ActiveXObject("UIHacks");
		UIHacks.MoveStyle = this.MoveStyle.Default;
		UIHacks.DisableSizing = false;
		UIHacks.BlockMaximize = false;
		UIHacks.MinSize = false;
		UIHacks.MaxSize = false;
	}
});

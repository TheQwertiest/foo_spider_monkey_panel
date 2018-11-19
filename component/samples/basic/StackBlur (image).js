window.DefinePanel("StackBlur", {author: "marc2003"});
include(`${fb.ComponentPath}docs\\Flags.js`);
include(`${fb.ComponentPath}docs\\Helpers.js`);

const img = gdi.Image(`${fb.ComponentPath}samples\\basic\\images\\post.jpg`);
const font = gdi.Font('Segoe UI', 16, 1);
let blur_img = null;
let radius = 20;

StackBlur(radius);

function StackBlur(radius) {
    blur_img = img.Clone(0, 0, img.Width, img.Height);
    blur_img.StackBlur(radius);
}

function on_paint(gr) {
    gr.DrawImage(blur_img, 0, 0, 600, 600, 0, 0, 600, 600);
    gr.DrawImage(img, 6, 30, 200, 200, 0, 0, 600, 600);
    // RGB function is defined in docs\\Helpers.js
    gr.FillSolidRect(0, 0, 600, 24, RGB(0, 0, 0));
    // DT_CENTER is defined in docs\\Flags.js
    gr.GdiDrawText('Scroll mouse to change radius: ' + radius, font, RGB(255, 255, 255), 0, 0, 600, 24, DT_CENTER);
}

function on_mouse_wheel(step) {
    radius += step * 5;
    if (radius < 2) {
        radius = 2;
    }
    if (radius > 254) {
        radius = 254;
    }
    StackBlur(radius);
    window.Repaint();
}

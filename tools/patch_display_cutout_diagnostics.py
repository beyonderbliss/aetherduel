from pathlib import Path

wrapper = Path("android-wrapper/src/main/java/io.github.edo9300.edopro/EpNativeActivity.java")
source = wrapper.read_text()

imports = "import android.view.WindowManager;\n"
assert source.count(imports) == 1, "Expected one WindowManager import"
source = source.replace(imports, imports + "import android.view.DisplayCutout;\n", 1)

native_decl = "\tpublic static native void errorDialogReturn();\n"
assert source.count(native_decl) == 1, "Expected one native declaration anchor"
source = source.replace(
    native_decl,
    native_decl + "\n\tpublic static native void reportDisplayCutoutInsets(int left, int top, int right, int bottom, int viewX, int viewY, int viewWidth, int viewHeight);\n",
    1,
)

on_create = "\t\tsuper.onCreate(savedInstanceState);\n"
assert source.count(on_create) == 1, "Expected one onCreate anchor"
source = source.replace(on_create, on_create + "\t\tinstallDisplayCutoutDiagnostics();\n", 1)

method_anchor = "\n\t@Override\n\tprotected void onResume() {"
assert source.count(method_anchor) == 1, "Expected one lifecycle insertion anchor"
method = """
\n\tprivate void installDisplayCutoutDiagnostics() {
\t\tif (Build.VERSION.SDK_INT < Build.VERSION_CODES.P)
\t\t\treturn;

\t\tfinal WindowManager.LayoutParams windowAttributes = getWindow().getAttributes();
\t\twindowAttributes.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_NEVER;
\t\tgetWindow().setAttributes(windowAttributes);

\t\tfinal View decorView = getWindow().getDecorView();
\t\tdecorView.setOnApplyWindowInsetsListener((view, insets) -> {
\t\t\tfinal DisplayCutout cutout = insets.getDisplayCutout();
\t\t\tfinal int left = cutout == null ? 0 : cutout.getSafeInsetLeft();
\t\t\tfinal int top = cutout == null ? 0 : cutout.getSafeInsetTop();
\t\t\tfinal int right = cutout == null ? 0 : cutout.getSafeInsetRight();
\t\t\tfinal int bottom = cutout == null ? 0 : cutout.getSafeInsetBottom();
\t\t\tfinal int[] location = new int[2];
\t\t\tview.getLocationOnScreen(location);
\t\t\treportDisplayCutoutInsets(left, top, right, bottom,
\t\t\t\t\tlocation[0], location[1], view.getWidth(), view.getHeight());
\t\t\treturn insets;
\t\t});
\t\tdecorView.post(decorView::requestApplyInsets);
\t}
"""
source = source.replace(method_anchor, method + method_anchor, 1)
wrapper.write_text(source)
print("Android window constrained to the display-cutout safe area.")

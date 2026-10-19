// @ts-check
/* Lucide icons: 02eb9161b57917ccaa262f1bf070507cddde3b08, icons/{x,minus,square,copy}.svg.
 * https://github.com/lucide-icons/lucide -- see LICENSES/Lucide.txt (ISC/Feather MIT).
 * SVG geometry is copied verbatim from those upstream files.
 */
(function attachX11WasmManagerIcons(global) {
  "use strict";
  const prefix = '<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true" focusable="false">';
  const icons = {
    close: '<path d="M18 6 6 18" /><path d="m6 6 12 12" />',
    minimize: '<path d="M5 12h14" />',
    maximize: '<rect width="18" height="18" x="3" y="3" rx="2" />',
    restore: '<rect width="14" height="14" x="8" y="8" rx="2" ry="2" /><path d="M4 16c-1.1 0-2-.9-2-2V4c0-1.1.9-2 2-2h10c1.1 0 2 .9 2 2" />',
  };
  /** @param {Document} document @param {keyof typeof icons} name @returns {Element} */
  global["X11WasmDOMPrivate"].managerIcon = (document, name) => {
    const template = document.createElement("template");
    template.innerHTML = prefix + icons[name] + "</svg>";
    return /** @type {Element} */ (template.content.firstElementChild);
  };
})(typeof globalThis !== "undefined" ? globalThis : window);

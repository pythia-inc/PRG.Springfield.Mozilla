/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

add_task(async function aria_attributes() {
  let win = await BrowserTestUtils.openNewBrowserWindow();
  is(
    win.FirefoxViewHandler.button.getAttribute("role"),
    "tab",
    "Firefox View button should have the 'tab' ARIA role"
  );
  await openFirefoxViewTab(win);
  isnot(
    win.FirefoxViewHandler.button.getAttribute("aria-controls"),
    "",
    "Firefox View button should have non-empty `aria-controls` attribute"
  );
  is(
    win.FirefoxViewHandler.button.getAttribute("aria-controls"),
    win.FirefoxViewHandler.tab.linkedPanel,
    "Firefox View button should refence the hidden tab's linked panel via `aria-controls`"
  );
  is(
    win.FirefoxViewHandler.button.getAttribute("aria-selected"),
    "true",
    'Firefox View button should have `aria-selected="true"` upon selecting it'
  );
  win.BrowserOpenTab();
  is(
    win.FirefoxViewHandler.button.getAttribute("aria-selected"),
    "false",
    'Firefox View button should have `aria-selected="false"` upon selecting a different tab'
  );
  await BrowserTestUtils.closeWindow(win);
});

add_task(async function load_opens_new_tab() {
  await withFirefoxView({}, async browser => {
    let win = browser.ownerGlobal;
    ok(win.FirefoxViewHandler.tab.selected, "Firefox View tab is selected");
    win.gURLBar.focus();
    win.gURLBar.value = "https://example.com";
    let newTabOpened = BrowserTestUtils.waitForEvent(
      win.gBrowser.tabContainer,
      "TabOpen"
    );
    EventUtils.synthesizeKey("KEY_Enter", {}, win);
    info(
      "Waiting for new tab to open from the address bar in the Firefox View tab"
    );
    await newTabOpened;
    assertFirefoxViewTab(win);
    ok(
      !win.FirefoxViewHandler.tab.selected,
      "Firefox View tab is not selected anymore (new tab opened in the foreground)"
    );
  });
});

add_task(async function homepage_new_tab() {
  await withFirefoxView({}, async browser => {
    let win = browser.ownerGlobal;
    ok(win.FirefoxViewHandler.tab.selected, "Firefox View tab is selected");
    let newTabOpened = BrowserTestUtils.waitForEvent(
      win.gBrowser.tabContainer,
      "TabOpen"
    );
    win.BrowserHome();
    info("Waiting for BrowserHome() to open a new tab");
    await newTabOpened;
    assertFirefoxViewTab(win);
    ok(
      !win.FirefoxViewHandler.tab.selected,
      "Firefox View tab is not selected anymore (home page opened in the foreground)"
    );
  });
});

add_task(async function number_tab_select_shortcut() {
  await withFirefoxView({}, async browser => {
    let win = browser.ownerGlobal;
    EventUtils.synthesizeKey(
      "1",
      AppConstants.MOZ_WIDGET_GTK ? { altKey: true } : { accelKey: true },
      win
    );
    ok(
      !win.FirefoxViewHandler.tab.selected,
      "Number shortcut to select the first tab skipped the Firefox View tab"
    );
  });
});

add_task(async function accel_w_behavior() {
  let win = await BrowserTestUtils.openNewBrowserWindow();
  await openFirefoxViewTab(win);
  EventUtils.synthesizeKey("w", { accelKey: true }, win);
  ok(!win.FirefoxViewHandler.tab, "Accel+w closed the Firefox View tab");
  await openFirefoxViewTab(win);
  win.gBrowser.selectedTab = win.gBrowser.visibleTabs[0];
  info(
    "Waiting for Accel+W in the only visible tab to close the window, ignoring the presence of the hidden Firefox View tab"
  );
  let windowClosed = BrowserTestUtils.windowClosed(win);
  EventUtils.synthesizeKey("w", { accelKey: true }, win);
  await windowClosed;
});

add_task(async function undo_close_tab() {
  let win = await BrowserTestUtils.openNewBrowserWindow();
  Services.obs.notifyObservers(null, "browser:purge-session-history");
  is(
    SessionStore.getClosedTabCount(win),
    0,
    "Closed tab count after purging session history"
  );

  let tab = await BrowserTestUtils.openNewForegroundTab(
    win.gBrowser,
    "about:about"
  );
  await TestUtils.waitForTick();

  let sessionUpdatePromise = BrowserTestUtils.waitForSessionStoreUpdate(tab);
  win.gBrowser.removeTab(tab);
  await sessionUpdatePromise;
  is(
    SessionStore.getClosedTabCount(win),
    1,
    "Closing about:about added to the closed tab count"
  );

  let viewTab = await openFirefoxViewTab(win);
  await TestUtils.waitForTick();
  sessionUpdatePromise = BrowserTestUtils.waitForSessionStoreUpdate(viewTab);
  closeFirefoxViewTab(win);
  await sessionUpdatePromise;
  is(
    SessionStore.getClosedTabCount(win),
    1,
    "Closing the Firefox View tab did not add to the closed tab count"
  );
  await BrowserTestUtils.closeWindow(win);
});

add_task(async function test_firefoxview_view_count() {
  const startViews = 2;
  await SpecialPowers.pushPrefEnv({
    set: [["browser.firefox-view.view-count", startViews]],
  });

  let tab = await openFirefoxViewTab(window);

  ok(
    SpecialPowers.getIntPref("browser.firefox-view.view-count") ===
      startViews + 1,
    "View count pref value is incremented when tab is selected"
  );

  BrowserTestUtils.removeTab(tab);
});

add_task(async function test_add_ons_cant_unhide_fx_view() {
  // Test that add-ons can't unhide the Firefox View tab by calling
  // browser.tabs.show(). See bug 1791770 for details.
  let win = await BrowserTestUtils.openNewBrowserWindow();
  let tab = await BrowserTestUtils.openNewForegroundTab(
    win.gBrowser,
    "about:about"
  );
  let viewTab = await openFirefoxViewTab(win);
  win.gBrowser.hideTab(tab);

  ok(tab.hidden, "Regular tab is hidden");
  ok(viewTab.hidden, "Firefox View tab is hidden");

  win.gBrowser.showTab(tab);
  win.gBrowser.showTab(viewTab);

  ok(!tab.hidden, "Add-on showed regular hidden tab");
  ok(viewTab.hidden, "Add-on did not show Firefox View tab");

  await BrowserTestUtils.closeWindow(win);
});

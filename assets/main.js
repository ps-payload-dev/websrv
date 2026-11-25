/* Copyright (C) 2024 idlesauce

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING. If not, see
<http://www.gnu.org/licenses/>.  */


// @ts-check

async function renderHomePage() {
	await scanHomebrews();
	let items = getHomebrewList();
	items.sort((a, b) => a.dir.split('/').pop().localeCompare(b.dir.split('/').pop()));

	let oldsandbox = document.getElementById("js-extension-sandbox");
	if (oldsandbox) {
		oldsandbox.remove();
	}

	let carouselItems = items.reduce(
		/**
		 * @param {Array} acc
		 * @param {*} item 
		 * @returns {CarouselItem[]} 
		 */
		(acc, item) => {
			let newItem = {
				mainText: item.filename,
				secondaryText: item.dir,
				imgPath: baseURL + "/fs" + item.dir + "/sce_sys/icon0.png"
			}
			if (item.filename.endsWith(".js")) {
				newItem.asyncInfo = async () => {
					let temp = await loadJsExtension(item.path);
					if (!temp) {
						return { result: { mainText: "Error", secondaryText: "Extension main() timed out." } };
					}



					return temp;
				}
			} else {
				newItem.onclick = () => {
					return { path: item.path };
				}

				newItem.options = [
					{
						text: "Remove from list",
						onclick: async () => {
							removeFromHomebrewList(item.path);
							await Globals.Router.handleHome();
						}
					},
					{
						text: "Launch with args",
						onclick: () => {
							let args = window.prompt("Enter args");
							if (args) {
								return { path: item.path, args: args };
							}
							return null;
						}
					},
					{
						text: "Launch with file",
						onclick: async () => {
							let result = await Globals.Router.pickFile(item.path.substring(0, item.path.lastIndexOf("/")));

							if (result) {
								return { path: item.path, args: result };
							}
						}
					}
				];
			}

			acc.push(newItem);
			return acc;
		}, []);

	carouselItems.push({
		mainText: "+",
		secondaryText: "Add more...",
		onclick: async () => {
			let result = await Globals.Router.pickFile(undefined, undefined, false);

			if (result) {
				addToHomebrewStore(result);
				await Globals.Router.handleHome();
			}
		}
	})

	await renderMainContentCarousel(carouselItems);
}

/**
 * @param {ReadableStream<Uint8Array>?} [hbldrLogStream] 
 */
async function renderLaunchedAppView(hbldrLogStream = null) {
	const content = document.getElementById("content");
	if (!content) {
		return;
	}
	const wrapper = document.createElement("div");
	wrapper.classList.add("launched-app-view");
	content.appendChild(wrapper);

	const msgWrapper = document.createElement("div");
	wrapper.appendChild(msgWrapper);

	const msgIngress = document.createElement("span");
	msgIngress.innerText = "The homebrew is running in the background, press ";
	msgWrapper.appendChild(msgIngress);

    const btnCircle = createSvgUseElement(PS_CIRCLE_ICON);
	msgWrapper.appendChild(btnCircle);

	const textCircle = document.createElement("span");
	textCircle.innerText = " to close this dialog or ";
	msgWrapper.appendChild(textCircle);

    const btnTriangle = createSvgUseElement(PS_TRIANGLE_ICON);
	msgWrapper.appendChild(btnTriangle);

	const textTriangle = document.createElement("span");
	textTriangle.innerText = " to go back to the launcher.";
	msgWrapper.appendChild(textTriangle);

	if (hbldrLogStream) {
		const terminal = document.createElement("div");
		terminal.classList.add("terminal");
		wrapper.appendChild(terminal);

		await (async () => {
			const fitAddon = new FitAddon.FitAddon();
			const term = new Terminal({
				convertEol: true,
				altClickMovesCursor: false,
				disableStdin: true,
				fontSize: 18,
			});

			try {
				const reader = hbldrLogStream.getReader();
				const decoder = new TextDecoder();

				term.open(terminal);
				term.loadAddon(fitAddon);
				fitAddon.fit();
				while (true) {
					const { done, value } = await reader.read();
					if (done) {
						break;
					}
					let decodedValue = decoder.decode(value);
					term.write(decodedValue);
				}
			} catch (error) {

			}
		})();
	}
}

window.onload = async function () {
	window.addEventListener("error", (event) => {
		let msg = event.message;
        if (event.filename && event.lineno) {
            msg += ' (' + event.filename + ':' + event.lineno + ')';
        }
        alert(msg);
	});
	window.addEventListener("unhandledrejection", (event) => {
        let msg = "Unhandled rejection: ";
        msg += event.reason;
        msg += '\n';
        msg += event.reason.stack;
        alert(msg);
	});

	const ver = await ApiClient.getVersion();
	if (ver.api > 0) {
		const verstr = ` ${ver.tag} (compiled at ${ver.date} ${ver.time})`;
		document.title += verstr;
	} else {
		document.title += ' (unknown version)';
	}

	registerExtensionMessagesListener();

	document.addEventListener("keydown", (event) => {
		if (event.keyCode === 118) {
			if (!confirm("Reset homebrew list?")) {
				return;
			}
			localStorage.removeItem(LOCALSTORE_HOMEBREW_LIST_KEY);
			window.location.href = "/";
		}

		if (event.keyCode === 116) {
			smoothScrollToElementIndex(0, true);
			event.preventDefault();
		}

		if (event.keyCode === 117) {
			let carouselInfo = getCurrentCarouselSelectionInfo();
			if (!carouselInfo) {
				return;
			}

			smoothScrollToElementIndex(carouselInfo.totalEntries - 1, true);
			event.preventDefault();
		}

		if (event.keyCode === 39 || event.keyCode === 37) {
			if (document.getElementById("modal-overlay")) {
				return;
			}

			let carouselInfo = getCurrentCarouselSelectionInfo();

			if (!carouselInfo) {
				return;
			}

			let currentElementIndex = carouselInfo.selectedIndex;
			if (!currentElementIndex) {
				currentElementIndex = 0;
			}

			if (event.keyCode === 39) {
				if (currentElementIndex === carouselInfo.totalEntries - 1 && 
					(Date.now() - Globals.lastCarouselLeftRightKeyDownTimestamp) > 200) {
					currentElementIndex = 0;
				} else {
					currentElementIndex++;
				}
			} else if (event.keyCode === 37) {
				if (currentElementIndex === 0 && 
					(Date.now() - Globals.lastCarouselLeftRightKeyDownTimestamp) > 200) {
					currentElementIndex = carouselInfo.totalEntries - 1;
				} else {
					currentElementIndex--;
				}
			}

			Globals.lastCarouselLeftRightKeyDownTimestamp = Date.now();
			smoothScrollToElementIndex(currentElementIndex, true);
		}
	});
	Globals.Router = new Router();
};


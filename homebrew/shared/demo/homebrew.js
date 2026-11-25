async function main() {
    let stateTest = "State is preserved";
    return {
        mainText: "demo",
        secondaryText: window.workingDir,
        imgPath: baseURL + "/fs/data/homebrew/demo/proxy-image.png", // the image in sce_sys/icon0.png wont show bc this overrides it
        onclick: async () => {

            await new Promise(resolve => setTimeout(resolve, 1000));

            let items = [
                {
                    mainText: "The Final Insect (Taurus 1993)",
                    imgPath: baseURL + "/fs/data/homebrew/demo/roms/1/img.jpg",
                    onclick: async () => {
                        await ApiClient.launchApp("/fs/data/homebrew/demo/demo.elf");
                        return true;
                    },
                    options: [
                        {
                            text: "Enter args",
                            onclick: () => { alert(prompt("Enter args")); }
                        }
                    ]
                },
                {
                    mainText: stateTest,
                    secondaryText: "bottom text",
                    onclick: () => {
                        return false;
                    }
                },
                {
                    mainText: "Pick file...",
                    onclick: async () => { alert("This alert is coming from the extension, path:" + await pickFile(window.workingDir)); } // pickfile arg = initial path, optional
                },
                {
                    mainText: "Pick directory...",
                    onclick: async () => { alert("This alert is coming from the extension, path:" + await pickDirectory(window.workingDir)); } // pickfile arg = initial path, optional
                }
            ];

            showCarousel(items);
        },
        options: [
            {
                text: "Error test",
                onclick: () => { throw new Error("Testing"); }
            },
            {
                text: "Option 2",
                onclick: () => { alert("Option 2"); }
            }
        ]

    };
}

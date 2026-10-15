use radiance::{Context, EffectNodeProps, ImageNodeProps, NodeProps};
use std::sync::{Arc, Mutex};

#[cfg(feature = "mpv")]
use radiance::MovieNodeProps;

#[derive(Debug, Clone)]
struct LibraryItem {
    name: String,
    node_props: NodeProps,
    custom: bool,
}

impl Ord for LibraryItem {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.name
            .cmp(&other.name)
            .then(node_type_index(&self.node_props).cmp(&node_type_index(&other.node_props)))
    }
}

impl PartialOrd for LibraryItem {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl Eq for LibraryItem {}

impl PartialEq for LibraryItem {
    // LibraryItems are equal if their name and type are equal
    fn eq(&self, other: &Self) -> bool {
        self.name == other.name
            && node_type_index(&self.node_props) == node_type_index(&other.node_props)
    }
}

// Arbitrary mapping of node type to an integer value
// for easy comparison
fn node_type_index(props: &NodeProps) -> u8 {
    match props {
        NodeProps::EffectNode(_) => 0,
        NodeProps::ImageNode(_) => 1,
        #[cfg(feature = "mpv")]
        NodeProps::MovieNode(_) => 2,
        NodeProps::UiBgNode(_) => 3,
        NodeProps::ScreenOutputNode(_) => 4,
        NodeProps::ProjectionMappedOutputNode(_) => 5,
        NodeProps::PlaceholderNode(_) => 6,
    }
}

#[derive(Debug, Default)]
struct LibraryMemory {
    textedit: String,
    cached_url: String, // Stores the last cached URL / path
    bookmarks: [String; 8], // Array for 8 quick bookmark slots
    contents: Vec<LibraryItem>,
    filtered_items: Vec<LibraryItem>,
    selected_index: Option<usize>,
    last_filter_text: String,
}

#[derive(Debug)]
pub enum LibraryResponse {
    None,
    Close,
    AddNode(NodeProps),
}

/// Renders the library widget
pub fn library_ui(ui: &mut egui::Ui, ctx: &Context, newly_opened: bool) -> LibraryResponse {
    let library_id = ui.make_persistent_id("library");

    let library_memory_arc = ui.ctx().memory_mut(|m| {
        m.data
            .get_temp_mut_or_default::<Arc<Mutex<LibraryMemory>>>(library_id)
            .clone()
    });

    // Keep an Arc clone to pass to the file picker thread
    let library_memory_ref = library_memory_arc.clone();

    // Acquire lock for main UI thread
    let mut library_memory = library_memory_arc.lock().unwrap();

    if newly_opened {
        library_memory.textedit.clear();
        library_memory.last_filter_text.clear();
        library_memory.contents = library_contents_from_filesystem(ctx);
        library_memory.filtered_items = library_memory.contents.clone();
        library_memory.selected_index = None;
    }

    let mut response = LibraryResponse::None;

    // Draw UI: Single input box with "Browse" and "Quick Cache/Paste" buttons
    let textbox_response = ui.horizontal(|ui| {
        // Space reserved for two action buttons on the right (30px + 30px + item spacing)
        let buttons_width = 65.0;
        let text_edit_width = (ui.available_width() - buttons_width - ui.spacing().item_spacing.x).max(50.0);

        let text_res = ui.add_sized(
            [text_edit_width, 20.0],
            egui::TextEdit::singleline(&mut library_memory.textedit),
        );

        // 1. File Dialog Button (Yellow background)
        let browse_btn = egui::Button::new("📁")
            .fill(egui::Color32::from_rgb(200, 160, 0)); // Warm yellow background

        if ui.add(browse_btn).on_hover_text("Browse files...").clicked() {
            let library_memory_clone = library_memory_ref.clone();

            std::thread::spawn(move || {
                if let Some(file) = rfd::FileDialog::new()
                    .add_filter("Video & Media", &["mp4", "mkv", "avi", "mov", "webm", "png", "jpg", "m3u", "m3u8"])
                    .pick_file() 
                {
                    if let Ok(mut mem) = library_memory_clone.lock() {
                        mem.textedit = file.to_string_lossy().to_string();
                    }
                }
            });
        }

        // 2. Stream URL Quick Cache / Paste Button (Green background when cache is populated)
        let cache_icon = if library_memory.textedit.trim().is_empty() {
            "📋" // Paste icon when text field is empty
        } else {
            "💾" // Save icon when text field contains a path/URL
        };

        let tooltip = if library_memory.textedit.trim().is_empty() {
            if library_memory.cached_url.is_empty() {
                "No URL saved in cache".to_string()
            } else {
                format!("Left-click: Paste cached URL ({})\nRight-click: Clear cache", library_memory.cached_url)
            }
        } else {
            if library_memory.cached_url.is_empty() {
                "Left-click: Save current URL to cache".to_string()
            } else {
                "Left-click: Save current URL to cache\nRight-click: Clear cache".to_string()
            }
        };

        let has_cached_url = !library_memory.cached_url.is_empty();
        
        let mut cache_btn = egui::Button::new(cache_icon);
        if has_cached_url {
            // Highlight in green if there is a URL saved in cache
            cache_btn = cache_btn.fill(egui::Color32::from_rgb(0, 130, 60));
        }

        let cache_btn_response = ui.add(cache_btn).on_hover_text(tooltip);

        // Left-click: Save to cache or Paste from cache
        if cache_btn_response.clicked() {
            let current_text = library_memory.textedit.trim().to_string();

            if current_text.is_empty() {
                // If input is empty, paste from cache
                if !library_memory.cached_url.is_empty() {
                    library_memory.textedit = library_memory.cached_url.clone();
                }
            } else {
                // If input has text, save it to cache
                library_memory.cached_url = current_text;
            }
        }

        // Right-click: Clear cached URL
        if cache_btn_response.secondary_clicked() {
            library_memory.cached_url.clear();
        }

        text_res
    }).inner;

    // Draw row for 8 Bookmark buttons (B1 .. B8)
    ui.horizontal(|ui| {
        let current_text = library_memory.textedit.trim().to_string();

        for i in 0..8 {
            let bookmark_val = &library_memory.bookmarks[i];
            let has_bookmark = !bookmark_val.is_empty();

            let bm_label = format!("B{}", i + 1);
            let mut bm_btn = egui::Button::new(&bm_label);

            if has_bookmark {
                // Highlight in green if the bookmark slot is populated
                bm_btn = bm_btn.fill(egui::Color32::from_rgb(0, 130, 60));
            }

            let tooltip = if current_text.is_empty() {
                if has_bookmark {
                    format!("Left-click: Paste bookmark {}\nRight-click: Clear bookmark {}", bookmark_val, bm_label)
                } else {
                    format!("Bookmark {} (Empty)\nType/paste a URL in text edit and left-click to save", bm_label)
                }
            } else {
                if has_bookmark {
                    format!("Left-click: Overwrite bookmark {}\nRight-click: Clear bookmark {}", bm_label, bm_label)
                } else {
                    format!("Left-click: Save current URL to bookmark {}", bm_label)
                }
            };

            let bm_response = ui.add(bm_btn).on_hover_text(tooltip);

            // Left-click: Save to bookmark or Paste from bookmark
            if bm_response.clicked() {
                if current_text.is_empty() {
                    if has_bookmark {
                        library_memory.textedit = bookmark_val.clone();
                    }
                } else {
                    library_memory.bookmarks[i] = current_text.clone();
                }
            }

            // Right-click: Clear single bookmark slot
            if bm_response.secondary_clicked() {
                library_memory.bookmarks[i].clear();
            }
        }
    });

    ui.add_space(4.0);

    // Filter items and find best match
    let filter_text = library_memory.textedit.to_lowercase();

    // Reset selected index and update filtered items if filter text changed
    if library_memory.last_filter_text != filter_text {
        library_memory.last_filter_text = filter_text.clone();
        if filter_text.is_empty() {
            library_memory.filtered_items = library_memory.contents.clone();
            library_memory.selected_index = None;
        } else {
            library_memory.filtered_items = {
                let mut filtered_items: Vec<_> = library_memory
                    .contents
                    .iter()
                    .filter(|item| {
                        let name_lower = item.name.to_lowercase();
                        name_lower.contains(&filter_text)
                    })
                    .cloned()
                    .collect();

                let mut custom_items = custom_library_items(&library_memory.textedit);
                custom_items.retain(|custom_item| !filtered_items.contains(custom_item));
                filtered_items.extend(custom_items);
                filtered_items
            };

            // Find best match
            library_memory.selected_index = library_memory
                .filtered_items
                .iter()
                .enumerate()
                .min_by_key(|(_, item)| {
                    let name_lower = item.name.to_lowercase();
                    (
                        item.custom, // Prefer non-custom items over custom ones
                        // Prefer substring to appear earlier in the name:
                        name_lower.find(&filter_text).unwrap_or(usize::MAX),
                    )
                })
                .map(|(i, _)| i);
        }
    }

    // Handle arrow keys
    let library_len = library_memory.filtered_items.len();
    if textbox_response.has_focus() {
        if ui.ctx().input(|i| i.key_pressed(egui::Key::ArrowDown)) {
            if let Some(selected_index) = &mut library_memory.selected_index {
                *selected_index = (*selected_index + 1).rem_euclid(library_len);
            } else if library_len > 0 {
                library_memory.selected_index = Some(0);
            }
        } else if ui.ctx().input(|i| i.key_pressed(egui::Key::ArrowUp)) {
            if let Some(selected_index) = &mut library_memory.selected_index {
                *selected_index = (*selected_index - 1).rem_euclid(library_len);
            } else if library_len > 0 {
                library_memory.selected_index = Some(library_len - 1);
            }
        }
    }

    egui::ScrollArea::vertical()
        .auto_shrink(false)
        .show(ui, |ui| {
            for (idx, item) in library_memory.filtered_items.iter().enumerate() {
                let icon = match item.node_props {
                    NodeProps::ImageNode(_) => "\u{1F5BC}",
                    #[cfg(feature = "mpv")]
                    NodeProps::MovieNode(_) => "\u{1F3A5}",
                    NodeProps::EffectNode(_) => "\u{2728}",
                    NodeProps::UiBgNode(_)
                    | NodeProps::ScreenOutputNode(_)
                    | NodeProps::ProjectionMappedOutputNode(_) => "\u{1F5B5}",
                    NodeProps::PlaceholderNode(_) => "\u{2754}",
                };
                let label = if item.custom {
                    (
                        format!("\u{27A1} {}", icon),
                        egui::RichText::new(item.name.clone()).italics(),
                    )
                } else {
                    (icon.to_string(), egui::RichText::new(item.name.clone()))
                };
                let is_selected = library_memory.selected_index == Some(idx);
                if ui.selectable_label(is_selected, label).clicked() {
                    response = LibraryResponse::AddNode(item.node_props.clone());
                }
            }

            ui.add_space(10.);
            ui.vertical_centered(|ui| {
                if ui.button("Open library folder").clicked() {
                    open::that_detached(ctx.library_directory())
                        .unwrap_or_else(|e| println!("Could not open library: {}", e));
                }
            });
            ui.add_space(10.);
        });

    // Handle response

    if newly_opened {
        textbox_response.request_focus();
    }

    if textbox_response.lost_focus() {
        if ui.ctx().input(|i| i.key_pressed(egui::Key::Enter)) {
            if let Some(selected_idx) = library_memory.selected_index {
                if let Some(item) = library_memory.filtered_items.get(selected_idx) {
                    response = LibraryResponse::AddNode(item.node_props.clone());
                }
            } else {
                response = LibraryResponse::Close;
            }
        } else if ui.ctx().input(|i| i.key_pressed(egui::Key::Escape)) {
            response = LibraryResponse::Close;
        }
    };

    response
}

fn library_contents_from_filesystem(ctx: &Context) -> Vec<LibraryItem> {
    let mut contents: Vec<_> = ctx
        .list_library_contents()
        .into_iter()
        .filter_map(|filename| filename_to_library_item(&filename))
        .collect();
    contents.sort();
    contents.extend([
        LibraryItem {
            name: "UiBg".to_owned(),
            node_props: NodeProps::UiBgNode(Default::default()),
            custom: false,
        },
        LibraryItem {
            name: "ScreenOutput".to_owned(),
            node_props: NodeProps::ScreenOutputNode(Default::default()),
            custom: false,
        },
        LibraryItem {
            name: "ProjectionMappedOutput".to_owned(),
            node_props: NodeProps::ProjectionMappedOutputNode(Default::default()),
            custom: false,
        },
    ]);
    contents
}

fn filename_to_library_item(filename: &str) -> Option<LibraryItem> {
    if filename.ends_with(".png") || filename.ends_with(".jpg") || filename.ends_with(".gif") {
        // Image files
        Some(LibraryItem {
            name: filename.to_string(),
            node_props: NodeProps::ImageNode(ImageNodeProps {
                name: filename.to_string(),
                ..ImageNodeProps::default()
            }),
            custom: false,
        })
    } else if filename.ends_with(".mp4") || filename.ends_with(".mkv") || filename.ends_with(".avi")
    {
        // Video files
        #[cfg(feature = "mpv")]
        {
            return Some(LibraryItem {
                name: filename.to_string(),
                node_props: NodeProps::MovieNode(MovieNodeProps {
                    name: filename.to_string(),
                    ..MovieNodeProps::default()
                }),
                custom: false,
            });
        }
        #[allow(unreachable_code)]
        None
    } else if filename.ends_with(".wgsl") {
        // Shader files (effect nodes) - strip the .wgsl extension
        let effect_name = filename.strip_suffix(".wgsl").unwrap();
        Some(LibraryItem {
            name: effect_name.to_string(),
            node_props: NodeProps::EffectNode(EffectNodeProps {
                name: effect_name.to_string(),
                ..EffectNodeProps::default()
            }),
            custom: false,
        })
    } else {
        None
    }
}

/// Returns library items based on the textbox text (e.g. for items that weren't discovered in the filesystem)
fn custom_library_items(input: &str) -> Vec<LibraryItem> {
    let mut items = vec![];

    if input.is_empty() {
        return items;
    }

    #[cfg(feature = "mpv")]
    items.push(LibraryItem {
        name: input.to_string(),
        node_props: NodeProps::MovieNode(MovieNodeProps {
            name: input.to_string(),
            ..MovieNodeProps::default()
        }),
        custom: true,
    });

    items.push(LibraryItem {
        name: input.to_string(),
        node_props: NodeProps::ImageNode(ImageNodeProps {
            name: input.to_string(),
            ..ImageNodeProps::default()
        }),
        custom: true,
    });

    items
}
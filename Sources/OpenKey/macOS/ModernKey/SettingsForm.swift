import AppKit
import SwiftUI

@available(macOS 13.0, *)
private final class SettingsFormModel: ObservableObject {
    let entries: [[String: Any]]
    private var lastState: [AnyHashable] = []
    init(_ entries: [[String: Any]]) {
        self.entries = entries
        lastState = controlState()
    }

    private func controlState() -> [AnyHashable] {
        func state(_ view: NSView) -> [AnyHashable] {
            if let control = view as? NSPopUpButton {
                return [control.isEnabled, control.indexOfSelectedItem] + control.itemTitles.map(AnyHashable.init)
            }
            if let control = view as? NSSegmentedControl { return [control.isEnabled, control.selectedSegment] }
            if let control = view as? NSSwitch { return [control.isEnabled, control.state.rawValue] }
            if let control = view as? NSButton { return [control.isEnabled, control.state.rawValue, control.title] }
            if let control = view as? NSTextField { return [control.isEnabled, control.stringValue] }
            if let stack = view as? NSStackView { return stack.arrangedSubviews.flatMap(state) }
            return []
        }
        return entries.flatMap { entry -> [AnyHashable] in
            if let rows = entry["rows"] as? [[String: Any]] {
                return rows.flatMap { ($0["control"] as? NSView).map(state) ?? [] }
            }
            return (entry["view"] as? NSView).map(state) ?? []
        }
    }

    func refresh() -> Bool {
        let next = controlState()
        guard next != lastState else { return false }
        lastState = next
        objectWillChange.send()
        return true
    }

    func send(_ control: NSControl) {
        guard let action = control.action else { return }
        NSApp.sendAction(action, to: control.target, from: control)
    }
}

@available(macOS 13.0, *)
private struct ExistingControl: NSViewRepresentable {
    let control: NSView

    func makeNSView(context: Context) -> NSView { control }
    func updateNSView(_ nsView: NSView, context: Context) {}

    func sizeThatFits(_ proposal: ProposedViewSize, nsView: NSView, context: Context) -> CGSize? {
        let fitting = nsView.fittingSize
        let intrinsic = nsView.intrinsicContentSize
        return CGSize(width: proposal.width ?? (intrinsic.width > 0 ? intrinsic.width : fitting.width),
                      height: intrinsic.height > 0 ? intrinsic.height : fitting.height)
    }
}

@available(macOS 13.0, *)
private struct NativeSettingsRow: View {
    let row: [String: Any]
    @ObservedObject var model: SettingsFormModel

    private var title: String { row["title"] as? String ?? "" }
    private var detail: String { row["detail"] as? String ?? "" }

    private var label: some View {
        VStack(alignment: .leading) {
            Text(title)
            if !detail.isEmpty {
                Text(detail).font(.subheadline).foregroundStyle(.secondary)
            }
        }
    }

    @ViewBuilder var body: some View {
        if let toggle = row["control"] as? NSSwitch {
            Toggle(isOn: Binding(get: { toggle.state == .on }, set: {
                toggle.state = $0 ? .on : .off
                model.send(toggle)
            })) { label }
            .disabled(!toggle.isEnabled)
            .accessibilityLabel(title)
            .accessibilityHint(detail)
        } else if let popup = row["control"] as? NSPopUpButton {
            Picker(selection: Binding(get: { popup.indexOfSelectedItem }, set: {
                popup.selectItem(at: $0)
                model.send(popup)
            })) {
                ForEach(popup.itemTitles.indices, id: \.self) { index in
                    Text(popup.itemTitles[index]).tag(index)
                }
            } label: { label }
            .disabled(!popup.isEnabled)
            .accessibilityLabel(title)
            .accessibilityHint(detail)
        } else if let segmented = row["control"] as? NSSegmentedControl {
            LabeledContent {
                Picker(title, selection: Binding(get: { segmented.selectedSegment }, set: {
                    segmented.selectedSegment = $0
                    model.send(segmented)
                })) {
                    ForEach(0..<segmented.segmentCount, id: \.self) {
                        Text(segmented.label(forSegment: $0) ?? "").tag($0)
                    }
                }.pickerStyle(.segmented).labelsHidden().fixedSize()
                .disabled(!segmented.isEnabled)
            } label: { label }
        } else if let control = row["control"] as? NSView {
            LabeledContent { ExistingControl(control: control).fixedSize() } label: { label }
        }
    }
}

@available(macOS 13.0, *)
private struct NativeSettingsForm: View {
    @ObservedObject var model: SettingsFormModel

    var body: some View {
        Form {
            ForEach(model.entries.indices, id: \.self) { index in
                let entry = model.entries[index]
                if let rows = entry["rows"] as? [[String: Any]] {
                    Section {
                        ForEach(rows.indices, id: \.self) { index in
                            NativeSettingsRow(row: rows[index], model: model)
                        }
                    } header: {
                        if let title = entry["title"] as? String, !title.isEmpty { Text(title) }
                    }
                } else if let text = entry["text"] as? String {
                    Text(text).font(.subheadline).foregroundStyle(.secondary)
                } else if let view = entry["view"] as? NSView {
                    if let text = view as? NSTextField, !text.isEditable {
                        Text(text.stringValue).font(.subheadline).foregroundStyle(.secondary)
                    } else {
                        ExistingControl(control: view)
                            .frame(maxWidth: .infinity, alignment: .leading)
                    }
                }
            }
        }
        .formStyle(.grouped)
    }
}

@available(macOS 13.0, *)
@objc(OKSettingsFormController)
final class SettingsFormController: NSViewController {
    private let model: SettingsFormModel

    @objc(initWithEntries:)
    init(entries: [[String: Any]]) {
        model = SettingsFormModel(entries)
        super.init(nibName: nil, bundle: nil)
    }

    required init?(coder: NSCoder) { nil }

    override func loadView() {
        view = NSHostingView(rootView: NativeSettingsForm(model: model))
    }

    @objc func refreshControls() -> Bool { model.refresh() }
}

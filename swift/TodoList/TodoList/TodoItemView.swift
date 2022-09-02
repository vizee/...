//
//  TodoItem.swift
//  TodoList
//
//  Created by vizee on 2022/8/20.
//

import SwiftUI

struct TodoItemView: View {
    @ObservedObject var data: AppData
    @Binding var todoIdx: Int
    @FocusState var titleFocused: Bool
    @State var checked = false
    @State var title = ""
    
    var body: some View {
        HStack {
            Button(action: {
                if !self.checked && self.data.editingIdx != todoIdx {
                    self.data.editingIdx = todoIdx
                    DispatchQueue.main.async {
                        titleFocused = true
                    }
                }
            }) {
                HStack {
                    Rectangle()
                        .fill(Color.accentColor)
                        .frame(width: 8)
                    VStack {
                        HStack {
                            if self.data.editingIdx == todoIdx {
                                TextField("What to do", text: $title)
                                    .font(.headline)
                                    .foregroundColor(Color("AnyText"))
                                    .textFieldStyle(.plain)
                                    .submitLabel(.done)
                                    .multilineTextAlignment(.leading)
                                    .lineLimit(1)
                                    .focused($titleFocused)
                                    .onSubmit {
                                        let title = self.title.trimmingCharacters(in: .whitespaces)
                                        if title == "" {
                                            self.title = self.data.todoItems[todoIdx].title
                                        } else {
                                            self.data.todoItems[todoIdx].title = title
                                            self.title = title
                                        }
                                        self.data.editingIdx = -1
                                    }
                            } else {
                                Text(self.title)
                                    .font(.headline)
                                    .foregroundColor(Color("AnyText"))
                                    .strikethrough(self.checked)
                                Spacer()
                            }
                        }.frame(height: 24)
                        Spacer().frame(height: 8)
                        HStack {
                            HStack {
                                Image(systemName: "clock")
                                    .resizable()
                                    .frame(width: 12, height: 12)
                                Text(shortDate.string(from: self.data.todoItems[todoIdx].dueDate))
                                    .font(.subheadline)
                            }
                            .onTapGesture {
                                if !checked {
                                    self.data.showDatePicker = todoIdx
                                }
                            }
                            Spacer()
                        }.foregroundColor(Color("TodoItemDate"))
                    }.padding()
                }
            }
            Button(action: {
                self.data.editingIdx = -1
                self.data.todoItems[todoIdx].checked.toggle()
                saveTodoItems(items: self.data.todoItems)
                self.checked = self.data.todoItems[todoIdx].checked
            }) {
                VStack {
                    Spacer()
                    Image(systemName: self.checked ? "checkmark.square.fill" : "square")
                        .resizable()
                        .frame(width: 24, height: 24)
                        .foregroundColor(self.checked ? .green : .gray)
                    Spacer()
                }.padding(.horizontal, 24)
            }.onAppear {
                let todo = self.data.todoItems[todoIdx]
                self.title = todo.title
                self.checked = todo.checked
            }
        }.background(Color("TodoItemBg"))
    }
}

#if DEBUG
struct TodoItemView_Previews: PreviewProvider {
    
    static var previews: some View {
        TodoItemView(data: { () -> AppData in
            let data = AppData()
            let item = TodoItem(title: "Yoshi", dueDate: Date())
            item.checked = false
            data.todoItems = [TodoItem(title: "Yoshi", dueDate: Date())]
            return data
        }(), todoIdx: .constant(0))
        .frame(height: 80)
        .preferredColorScheme(.dark)
    }
}
#endif

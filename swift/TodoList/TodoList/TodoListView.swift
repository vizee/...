//
//  TodoListView.swift
//  TodoList
//
//  Created by vizee on 2022/8/27.
//

import SwiftUI

struct TodoListView: View {
    @ObservedObject var data: AppData
    @State var newItem: String = ""
    
    var body: some View {
        NavigationView {
            VStack {
                ScrollView {
                    ForEach(data.todoItems) { todo in
                        VStack {
                            if todo.idx == 0 || shortDate.string(from: todo.dueDate) != shortDate.string(from: self.data.todoItems[todo.idx - 1].dueDate) {
                                HStack {
                                    Text(mediumDate.string(from: todo.dueDate))
                                    Spacer()
                                }
                            }
                            TodoItemView(data: self.data, todoIdx: .constant(todo.idx))
                        }
                        .padding(.horizontal, 8).padding(.vertical, 4)
                    }
                }
                .navigationTitle(Text("Todo List"))
                .edgesIgnoringSafeArea(.bottom)
                HStack {
                    HStack {
                        Spacer().frame(width: 8)
                        Image(systemName: "plus")
                            .resizable()
                            .frame(width: 16, height: 16)
                            .foregroundColor(Color.secondary)
                        Spacer().frame(width: 8)
                        TextField("What to do", text: $newItem)
                            .frame(height: 48)
                            .font(.body)
                            .foregroundColor(Color("AnyText"))
                            .textFieldStyle(.plain)
                            .multilineTextAlignment(.leading)
                            .lineLimit(1)
                            .submitLabel(.done)
                            .onSubmit {
                                let title = newItem.trimmingCharacters(in: .whitespaces)
                                if title != "" {
                                    self.data.todoItems.append(TodoItem(title: title, dueDate: Date()))
                                    self.data.sortTodoItems()
                                    saveTodoItems(items: self.data.todoItems)
                                }
                                self.newItem = ""
                            }
                        Spacer().frame(width: 12)
                    }.background(Color("TodoItemBg"))
                        .padding(8)
                }
            }
        }
    }
}

#if DEBUG
struct TodoListView_Previews: PreviewProvider {
    static var previews: some View {
        TodoListView(data: { () -> AppData in
            let data = AppData()
            data.todoItems = [
                TodoItem(title: "Yoshi0", dueDate: Date()),
                TodoItem(title: "Yoshi1", dueDate: Date()),
                TodoItem(title: "Yoshi2", dueDate: Date()),
                TodoItem(title: "Yoshi3", dueDate: Date()),
                TodoItem(title: "Yoshi4", dueDate: Date()),
                TodoItem(title: "Yoshi5", dueDate: Date()),
                TodoItem(title: "Yoshi6", dueDate: Date()),
                TodoItem(title: "Go to work", dueDate: ISO8601DateFormatter().date(from: "2022-08-29T00:00:00Z") ?? Date())
            ]
            data.todoItems[0].checked = true
            data.sortTodoItems()
            return data
        }())
        .preferredColorScheme(.dark)
        
    }
}
#endif

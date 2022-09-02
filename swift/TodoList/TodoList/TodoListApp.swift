//
//  TodoListApp.swift
//  TodoList
//
//  Created by vizee on 2022/8/20.
//

import SwiftUI

class AppData: ObservableObject {
    @Published var todoItems: [TodoItem] = []
    @Published var editingIdx = -1
    @Published var showDatePicker = -1
    
    func sortTodoItems() {
        self.todoItems.sort(by: { $0.dueDate.compare($1.dueDate) == .orderedAscending })
        todoItems.enumerated().forEach({ $1.idx = $0 })
    }
}

@main
struct TodoListApp: App {
    var body: some Scene {
        WindowGroup {
            HomeView(data: { () -> AppData in
                let data = AppData()
                let todoItems =  loadTodoItems() ?? []
                for item in todoItems {
                    if !item.checked {
                        data.todoItems.append(item)
                    }
                }
                data.sortTodoItems()
                return data
            }())
        }
    }
}

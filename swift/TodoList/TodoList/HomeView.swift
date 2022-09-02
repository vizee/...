//
//  HomeView.swift
//  TodoList
//
//  Created by vizee on 2022/8/20.
//

import SwiftUI

struct HomeView: View {
    @ObservedObject var data: AppData
    
    var body: some View {
        ZStack {
            TodoListView(data: self.data)
                .blur(radius: self.data.showDatePicker != -1 ? 10 : 0)
                .allowsHitTesting(self.data.showDatePicker == -1)
            if self.data.showDatePicker != -1 {
                DueDatePicker(date: self.data.todoItems[data.showDatePicker].dueDate, confirm: {
                    self.data.todoItems[data.showDatePicker].dueDate = $0
                    self.data.sortTodoItems()
                    saveTodoItems(items: self.data.todoItems)
                    self.data.showDatePicker = -1
                }, dismiss: {
                    self.data.showDatePicker = -1
                })
            }
        }
    }
}

#if DEBUG
struct HomeView_Previews: PreviewProvider {
    static var previews: some View {
        HomeView(data: AppData())
    }
}
#endif

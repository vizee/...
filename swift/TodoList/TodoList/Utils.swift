//
//  Utils.swift
//  TodoList
//
//  Created by vizee on 2022/8/29.
//

import Foundation

let shortDate =  { () -> DateFormatter in let f = DateFormatter(); f.dateStyle = .short; return f }()
let mediumDate = { () -> DateFormatter in let f = DateFormatter(); f.dateStyle = .medium; return f }()


func saveTodoItems(items: [TodoItem]) {
    do {
        let todoItemsData = try NSKeyedArchiver.archivedData(withRootObject: items, requiringSecureCoding: false)
        UserDefaults.standard.set(todoItemsData, forKey: "todoItems")
    } catch let err {
        print("save data \(err)")
    }
}

func loadTodoItems() -> [TodoItem]? {
    if let data = UserDefaults.standard.data(forKey: "todoItems") {
        return NSKeyedUnarchiver.unarchiveObject(with: data) as? [TodoItem]
    } else {
        return nil
    }
}

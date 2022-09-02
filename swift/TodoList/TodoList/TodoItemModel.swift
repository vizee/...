//
//  TodoItemModel.swift
//  TodoList
//
//  Created by vizee on 2022/8/20.
//

import Foundation

class TodoItem: NSObject, NSCoding, Identifiable {
    var title: String
    var dueDate: Date
    var checked: Bool
    var idx: Int = 0
    
    func encode(with coder: NSCoder) {
        coder.encode(self.title, forKey: "title")
        coder.encode(self.dueDate, forKey: "dueDate")
        coder.encode(self.checked, forKey: "checked")
    }
    
    required init?(coder: NSCoder) {
        self.title = coder.decodeObject(forKey: "title") as? String ?? ""
        self.dueDate = coder.decodeObject(forKey: "dueDate") as? Date ?? Date()
        self.checked = coder.decodeBool(forKey: "checked")
    }
    
    init(title: String, dueDate: Date) {
        self.title = title
        self.dueDate = dueDate
        self.checked = false
    }
}

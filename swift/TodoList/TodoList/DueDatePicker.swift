//
//  DueDatePicker.swift
//  TodoList
//
//  Created by vizee on 2022/9/2.
//

import SwiftUI

struct DueDatePicker: View {
    @State var date: Date
    var confirm: (Date) -> Void
    var dismiss: () -> Void
    
    var body: some View {
        VStack {
            Spacer()
            HStack {
                Button(action: dismiss) {
                    Image(systemName: "xmark")
                        .resizable()
                        .frame(width: 24, height: 24)
                        .foregroundColor(Color.red)
                }.padding(12)
                Spacer()
                Button(action: { confirm(date) }) {
                    Image(systemName: "checkmark")
                        .resizable()
                        .frame(width: 24, height: 24)
                        .foregroundColor(Color.green)
                }.padding(12)
            }
            DatePicker("", selection: $date, displayedComponents: .date)
                .datePickerStyle(.wheel)
                .labelsHidden()
                .padding()
        }
    }
}

#if DEBUG
struct DueDatePicker_Previews: PreviewProvider {
    static var previews: some View {
        DueDatePicker(date: Date(), confirm: { (date)  in}, dismiss: {})
            .preferredColorScheme(.light)
    }
}
#endif

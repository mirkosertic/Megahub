import { describe, it, expect, vi } from 'vitest'
import { definition } from '../../src/components/blockly/ui_show_value.js'

function makeBlock(fields = {}, values = {}, statements = {}, extra = {}) {
    return {
        getFieldValue: (name) => fields[name] ?? null,
        id: extra.id ?? 'test-block-id',
        outputConnection: extra.outputConnection ?? null,
        ...extra
    }
}

function makeGenerator(values = {}, statements = {}) {
    return {
        valueToCode: (block, name, _order) => values[name] ?? '',
        statementToCode: (block, name) => statements[name] ?? ''
    }
}

describe('ui_show_value block definition', () => {
    it('has category "UI"', () => {
        expect(definition.category).toBe('UI')
    })

    it('has type "ui_show_value"', () => {
        expect(definition.blockdefinition.type).toBe('ui_show_value')
    })

    it('has previousStatement and nextStatement', () => {
        expect(definition.blockdefinition.previousStatement).toBe(true)
        expect(definition.blockdefinition.nextStatement).toBe(true)
    })

    it('has inputsForToolbox with shadow type "text"', () => {
        expect(definition.inputsForToolbox).toBeDefined()
        expect(definition.inputsForToolbox.VALUE).toBeDefined()
        expect(definition.inputsForToolbox.VALUE.shadow.type).toBe('text')
    })
})

describe('ui_show_value generator', () => {
    it('generates correct output for LABEL=Speed, STYLE=FORMAT_SIMPLE, VALUE=42', () => {
        const block = makeBlock({ LABEL: 'Speed', STYLE: 'FORMAT_SIMPLE' })
        const gen = makeGenerator({ VALUE: '42' })
        const code = definition.generator(block, gen)
        expect(code).toBe('ui.showvalue("Speed", FORMAT_SIMPLE, 42)\n')
    })

    it('includes label in output', () => {
        const block = makeBlock({ LABEL: 'Temp', STYLE: 'FORMAT_SIMPLE' })
        const gen = makeGenerator({ VALUE: '25' })
        const code = definition.generator(block, gen)
        expect(code).toContain('"Temp"')
    })

    it('includes style in output', () => {
        const block = makeBlock({ LABEL: 'X', STYLE: 'FORMAT_SIMPLE' })
        const gen = makeGenerator({ VALUE: '0' })
        const code = definition.generator(block, gen)
        expect(code).toContain('FORMAT_SIMPLE')
    })

    it('ends with newline', () => {
        const block = makeBlock({ LABEL: 'L', STYLE: 'FORMAT_SIMPLE' })
        const gen = makeGenerator({ VALUE: 'v' })
        const code = definition.generator(block, gen)
        expect(code).toMatch(/\n$/)
    })
})

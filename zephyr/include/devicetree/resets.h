/**
 * @file
 * @brief Reset Devicetree macro public API header file.
 */

/*
 * Copyright (c) 2020, Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_RESETS_H_
#define ZEPHYR_INCLUDE_DEVICETREE_RESETS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-resets Devicetree Clocks API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get a label property from the node referenced by a pwms
 *        property at an index
 *
 * It's an error if the reset controller node referenced by the
 * phandle in node_id's resets property at index "idx" has no label
 * property.
 *
 * Example devicetree fragment:
 *
 *     rst1: reset-controller@... {
 *             label = "CLK_1";
 *     };
 *
 *     rst2: reset-controller@... {
 *             label = "CLK_2";
 *     };
 *
 *     n: node {
 *             resets = <&rst1 10 20>, <&rst2 30 40>;
 *     };
 *
 * Example usage:
 *
 *     DT_RESETS_LABEL_BY_IDX(DT_NODELABEL(n), 1) // "CLK_2"
 *
 * @param node_id node identifier for a node with a resets property
 * @param idx logical index into resets property
 * @return the label property of the node referenced at index "idx"
 * @see DT_PROP_BY_PHANDLE_IDX()
 */
#define DT_RESETS_LABEL_BY_IDX(node_id, idx) \
	DT_PROP_BY_PHANDLE_IDX(node_id, resets, idx, label)

/**
 * @brief Get a label property from a resets property by name
 *
 * It's an error if the reset controller node referenced by the
 * phandle in node_id's resets property at the element named "name"
 * has no label property.
 *
 * Example devicetree fragment:
 *
 *     rst1: reset-controller@... {
 *             label = "CLK_1";
 *     };
 *
 *     rst2: reset-controller@... {
 *             label = "CLK_2";
 *     };
 *
 *     n: node {
 *             resets = <&rst1 10 20>, <&rst2 30 40>;
 *             reset-names = "alpha", "beta";
 *     };
 *
 * Example usage:
 *
 *     DT_RESETS_LABEL_BY_NAME(DT_NODELABEL(n), beta) // "CLK_2"
 *
 * @param node_id node identifier for a node with a resets property
 * @param name lowercase-and-underscores name of a resets element
 *             as defined by the node's reset-names property
 * @return the label property of the node referenced at the named element
 * @see DT_PHANDLE_BY_NAME()
 */
#define DT_RESETS_LABEL_BY_NAME(node_id, name) \
	DT_PROP(DT_PHANDLE_BY_NAME(node_id, resets, name), label)

/**
 * @brief Equivalent to DT_RESETS_LABEL_BY_IDX(node_id, 0)
 * @param node_id node identifier for a node with a resets property
 * @return the label property of the node referenced at index 0
 * @see DT_RESETS_LABEL_BY_IDX()
 */
#define DT_RESETS_LABEL(node_id) DT_RESETS_LABEL_BY_IDX(node_id, 0)

/**
 * @brief Get a reset specifier's cell value at an index
 *
 * Example devicetree fragment:
 *
 *     rst1: reset-controller@... {
 *             compatible = "vnd,reset";
 *             #reset-cells = < 2 >;
 *     };
 *
 *     n: node {
 *             resets = < &rst1 10 20 >, < &rst1 30 40 >;
 *     };
 *
 * Bindings fragment for the vnd,reset compatible:
 *
 *     reset-cells:
 *       - bus
 *       - bits
 *
 * Example usage:
 *
 *     DT_RESETS_CELL_BY_IDX(DT_NODELABEL(n), 0, bus) // 10
 *     DT_RESETS_CELL_BY_IDX(DT_NODELABEL(n), 1, bits) // 40
 *
 * @param node_id node identifier for a node with a resets property
 * @param idx logical index into resets property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index "idx"
 * @see DT_PHA_BY_IDX()
 */
#define DT_RESETS_CELL_BY_IDX(node_id, idx, cell) \
	DT_PHA_BY_IDX(node_id, resets, idx, cell)

/**
 * @brief Get a reset specifier's cell value by name
 *
 * Example devicetree fragment:
 *
 *     rst1: reset-controller@... {
 *             compatible = "vnd,reset";
 *             #reset-cells = < 2 >;
 *     };
 *
 *     n: node {
 *             resets = < &rst1 10 20 >, < &rst1 30 40 >;
 *             reset-names = "alpha", "beta";
 *     };
 *
 * Bindings fragment for the vnd,reset compatible:
 *
 *     reset-cells:
 *       - bus
 *       - bits
 *
 * Example usage:
 *
 *     DT_RESETS_CELL_BY_NAME(DT_NODELABEL(n), alpha, bus) // 10
 *     DT_RESETS_CELL_BY_NAME(DT_NODELABEL(n), beta, bits) // 40
 *
 * @param node_id node identifier for a node with a resets property
 * @param name lowercase-and-underscores name of a resets element
 *             as defined by the node's reset-names property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value in the specifier at the named element
 * @see DT_PHA_BY_NAME()
 */
#define DT_RESETS_CELL_BY_NAME(node_id, name, cell) \
	DT_PHA_BY_NAME(node_id, resets, name, cell)

/**
 * @brief Equivalent to DT_RESETS_CELL_BY_IDX(node_id, 0, cell)
 * @param node_id node identifier for a node with a resets property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index 0
 * @see DT_RESETS_CELL_BY_IDX()
 */
#define DT_RESETS_CELL(node_id, cell) DT_RESETS_CELL_BY_IDX(node_id, 0, cell)

/**
 * @brief Get a label property from a DT_DRV_COMPAT instance's resets
 *        property at an index
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into resets property
 * @return the label property of the node referenced at index "idx"
 * @see DT_RESETS_LABEL_BY_IDX()
 */
#define DT_INST_RESETS_LABEL_BY_IDX(inst, idx) \
	DT_RESETS_LABEL_BY_IDX(DT_DRV_INST(inst), idx)

/**
 * @brief Get a label property from a DT_DRV_COMPAT instance's resets
 *        property by name
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of a resets element
 *             as defined by the node's reset-names property
 * @return the label property of the node referenced at the named element
 * @see DT_RESETS_LABEL_BY_NAME()
 */
#define DT_INST_RESETS_LABEL_BY_NAME(inst, name) \
	DT_RESETS_LABEL_BY_NAME(DT_DRV_INST(inst), name)

/**
 * @brief Equivalent to DT_INST_RESETS_LABEL_BY_IDX(inst, 0)
 * @param inst DT_DRV_COMPAT instance number
 * @return the label property of the node referenced at index 0
 * @see DT_RESETS_LABEL_BY_IDX()
 */
#define DT_INST_RESETS_LABEL(inst) DT_INST_RESETS_LABEL_BY_IDX(inst, 0)

/**
 * @brief Get a DT_DRV_COMPAT instance's reset specifier's cell value
 *        at an index
 * @param inst DT_DRV_COMPAT instance number
 * @param idx logical index into resets property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value at index "idx"
 * @see DT_RESETS_CELL_BY_IDX()
 */
#define DT_INST_RESETS_CELL_BY_IDX(inst, idx, cell) \
	DT_RESETS_CELL_BY_IDX(DT_DRV_INST(inst), idx, cell)

/**
 * @brief Get a DT_DRV_COMPAT instance's reset specifier's cell value by name
 * @param inst DT_DRV_COMPAT instance number
 * @param name lowercase-and-underscores name of a resets element
 *             as defined by the node's reset-names property
 * @param cell lowercase-and-underscores cell name
 * @return the cell value in the specifier at the named element
 * @see DT_RESETS_CELL_BY_NAME()
 */
#define DT_INST_RESETS_CELL_BY_NAME(inst, name, cell) \
	DT_RESETS_CELL_BY_NAME(DT_DRV_INST(inst), name, cell)

/**
 * @brief Equivalent to DT_INST_RESETS_CELL_BY_IDX(inst, 0, cell)
 * @param inst DT_DRV_COMPAT instance number
 * @param cell lowercase-and-underscores cell name
 * @return the value of the cell inside the specifier at index 0
 */
#define DT_INST_RESETS_CELL(inst, cell) \
	DT_INST_RESETS_CELL_BY_IDX(inst, 0, cell)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_RESETS_H_ */
